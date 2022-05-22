
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/general.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>

#include <string.h>

////////////////////////
//
// DEINIT
//
////////////////////////
void FontRenderSystem::deInit()
{
    destroyImage(textImage);

    destroyDescriptor(pipelinesWithDescriptor.descriptor);
    destroyPipeline(pipelinesWithDescriptor);

    destroySampler(textureSampler);

    destroyBuffer(letterIndexBuffer);

    destroyShaderModule(vertexShader);
    destroyShaderModule(fragShader);
}



bool FontRenderSystem::init(std::string_view fontFilename)
{
#if 1
    PodVector<char> data;
    if (!loadBytes(fontFilename, data))
    {
        printf("Failed to load file: %s\n", fontFilename.data());
        return false;
    }

    // Create buffers
    {
        vertexShader = loadShader("assets/shader/vulkan_new/texturedquad.vert.spv");
        ASSERT(vertexShader);

        fragShader = loadShader("assets/shader/vulkan_new/texturedquad.frag.spv");
        ASSERT(fragShader);

        letterDataBufferHandle = vulk.uniformBufferManager.reserveHandle();

        letterIndexBuffer = createBuffer(1 * 1024 * 1024,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index font character buffer");

        {
            uint32_t offset = 0;
            PodVector<uint32_t> indices;
            indices.resize(6 * MAX_LETTERS);
            for (int i = 0; i < MAX_LETTERS; ++i)
            {
                indices[size_t(i) * 6 + 0] = i * 4 + 1;
                indices[size_t(i) * 6 + 1] = i * 4 + 0;
                indices[size_t(i) * 6 + 2] = i * 4 + 2;

                indices[size_t(i) * 6 + 3] = i * 4 + 2;
                indices[size_t(i) * 6 + 4] = i * 4 + 0;
                indices[size_t(i) * 6 + 5] = i * 4 + 3;
            }
            offset = uploadToScratchbuffer((void*)indices.data(), size_t(sizeof(indices[0]) * indices.size()), offset);
            uploadScratchBufferToGpuBuffer(letterIndexBuffer, offset);
        }
    }

    //Font image data
    {
        PodVector<uint8_t> fontPic;
        fontPic.resize((128 - 32) * 8 * 12 * 4);

        // Note save order is a bit messed up!!! Since the file has one char 8x12 then next
        uint32_t index = 0;
        for (int y = 0; y < 12; ++y)
        {
            for (int charIndex = 0; charIndex < 128 - 32; ++charIndex)
            {
                uint8_t p = data[y + size_t(charIndex) * 12];
                for (int x = 0; x < 8; ++x)
                {
                    uint8_t bitColor = uint8_t((p >> x) & 1) * 255;
                    fontPic[size_t(index) * 4 + 0] = bitColor;
                    fontPic[size_t(index) * 4 + 1] = bitColor;
                    fontPic[size_t(index) * 4 + 2] = bitColor;
                    fontPic[size_t(index) * 4 + 3] = bitColor;

                    ++index;
                }
            }
        }
        const int textureWidth = 8 * (128 - 32);
        const int textureHeight = 12;

        textImage = createImage(textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            "TextImage");

        updateImageWithData(textureWidth, textureHeight, 4u,
            textImage,
            (uint32_t)fontPic.size(), (void*)fontPic.data());

        VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        textureSampler = createSampler(samplerInfo);
        if(!textureSampler)
        {
            printf("Failed to create sampler for font rendering");
            return false;
        }
    }

    // Create pipelines
    {
        PipelineWithDescriptors &pipeline = pipelinesWithDescriptor;
        pipeline.descriptorSetLayouts = PodVector<DescriptorSetLayout>({
                DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u },
                DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u },
                DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2u }
        });



        if (!createPipelineLayout(pipeline, VK_SHADER_STAGE_ALL_GRAPHICS))
        {
            printf("Failed to create pipelinelayout!\n");
            return false;
        }

        pipeline.pipeline = createGraphicsPipeline(
            vertexShader, fragShader,
            pipeline.pipelineLayout,
            { vulk.defaultColorFormat },
            {}
        );

        pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
        {
            DescriptorInfo(vulk.renderFrameBufferHandle),
            DescriptorInfo(letterDataBufferHandle),
            DescriptorInfo(textImage.imageView, VK_IMAGE_LAYOUT_GENERAL, textureSampler),
        });

        pipeline.descriptor = createDescriptor(pipeline.descriptorSetLayouts, pipeline.descriptorSetLayout);
        if(!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }
#endif

    return true;
}




void FontRenderSystem::addText(std::string_view text, Vector2 pos, Vec2 charSize, const Vector4& color)
{
    ASSERT(vertData.size() + text.length() < MAX_LETTERS);
    uint16_t charWidth = uint16_t(charSize.x);
    uint16_t charHeight = uint16_t(charSize.y);

    for(const char c : text)
    {
        GPUVertexData vdata;
        vdata.color = getColor(color.x, color.y, color.z, color.w);
        vdata.pixelSizeX = charWidth;
        vdata.pixelSizeY = charHeight;
        vdata.pos = pos;

        if(c >= 32 && c <= 127)
        {
            uint32_t letter = c - 32;
            static constexpr float LetterTextureWidth = 128.0f - 32.0f;
            vdata.uvStart = Vec2(float(letter) / LetterTextureWidth, 0.0f);
            vdata.uvSize = Vec2(1.0f / LetterTextureWidth, 1.0f);

            vertData.emplace_back(vdata);
        }
        pos.x += charWidth;
    }
}


void FontRenderSystem::update()
{
    if (vertData.size() == 0)
        return;

    addToCopylist(sliceFromPodVectorBytes( vertData ), letterDataBufferHandle);
}

void FontRenderSystem::reset()
{
    vertData.clear();
}

void FontRenderSystem::render(Image &image)
{
    if (vertData.size() == 0 || vulk.commandBuffer == nullptr)
        return;

    VkRect2D renderArea = { .extent = {.width = vulk.swapchain.width, .height = vulk.swapchain.height } };
    VkViewport viewPort = { 0.0f, float(vulk.swapchain.height), float(vulk.swapchain.width), -float(vulk.swapchain.height), 0.0f, 1.0f };
    VkRect2D scissors = { { 0, 0 }, { uint32_t(vulk.swapchain.width), uint32_t(vulk.swapchain.height) } };


    const VkRenderingAttachmentInfo colorAttachmentInfo[]{
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = image.imageView,
            .imageLayout = image.layout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        },

    };

    const VkRenderingInfo renderInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = renderArea,
        .layerCount = 1,
        .colorAttachmentCount = ARRAYSIZES(colorAttachmentInfo),
        .pColorAttachments = colorAttachmentInfo,
        .pDepthAttachment = nullptr,
    };

    vkCmdBeginRendering(vulk.commandBuffer, &renderInfo);

    insertDebugRegion("FontRender", Vec4(1.0f, 1.0f, 0.0f, 1.0f));
    vkCmdSetViewport(vulk.commandBuffer, 0, 1, &viewPort);
    vkCmdSetScissor(vulk.commandBuffer, 0, 1, &scissors);

    bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinesWithDescriptor);
    vkCmdBindIndexBuffer(vulk.commandBuffer, letterIndexBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(vulk.commandBuffer, uint32_t(vertData.size() * 6), 1, 0, 0, 0);

    vkCmdEndRendering(vulk.commandBuffer);
}



