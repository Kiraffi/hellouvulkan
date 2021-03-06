
#include <container/podvector.h>
#include <container/vector.h>

#include <core/file.h>
#include <core/general.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>

#include <myvulkan/myvulkan.h>

////////////////////////
//
// DEINIT
//
////////////////////////
void FontRenderSystem::deInit()
{
    destroyImage(textImage);
    destroyDescriptor(pipeline.descriptor);
    destroyPipeline(pipeline);

    destroySampler(textureSampler);

    destroyBuffer(letterIndexBuffer);
}



bool FontRenderSystem::init(const char *fontFilename)
{
    PodVector<uint8_t> data;
    if (!loadBytes(fontFilename, data.getBuffer()))
    {
        printf("Failed to load file: %s\n", fontFilename);
        return false;
    }

    // Create buffers
    {
        for(uint32_t i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            letterDataBufferHandle[i] = vulk->uniformBufferManager.reserveHandle();
        }

        letterIndexBuffer = createBuffer(1 * 1024 * 1024,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index font character buffer");

        {
            uint32_t offset = 0;
            PodVector<uint32_t> indices;
            indices.uninitializedResize(6 * MAX_LETTERS);
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

        if (!createImage(textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            "TextImage", textImage))
        {
            printf("Failed to create image for text!\n");
            return false;
        }

        updateImageWithData(textureWidth, textureHeight, 4u,
            textImage,
            (uint32_t)fontPic.size(), (void*)fontPic.data());

        VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        textureSampler = createSampler(samplerInfo);
        if (!textureSampler)
        {
            printf("Failed to create sampler for font rendering");
            return false;
        }
    }

    // Create pipelines
    {
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);
        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);

        pipeline.renderPass = createRenderPass(
                { RenderTarget{ .format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD } },
                {});
        ASSERT(pipeline.renderPass);
        if(!pipeline.renderPass)
            return false;

        VkPipelineColorBlendAttachmentState rgbaAtt{ .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        if(!createGraphicsPipeline(
            getShader(ShaderType::TexturedQuadVert), getShader(ShaderType::TexturedQuadFrag),
            { { rgbaAtt } }, {}, pipeline, "Font renderer"))
        {
            printf("failed to create graphics pipeline\n");
            return false;
        }
        for(uint32_t i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo> {
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),
                DescriptorInfo(letterDataBufferHandle[i]),
                DescriptorInfo(textImage.imageView, VK_IMAGE_LAYOUT_GENERAL, textureSampler),
            };
        }
        if(!updateBindDescriptorSet(pipeline))
            return false;

    }

    return true;
}

void FontRenderSystem::setRenderTarget(Image& image)
{
    ASSERT(createFramebuffer(pipeline, { image }));
}



void FontRenderSystem::addText(const char *text, Vector2 pos, Vec2 charSize, const Vector4& color)
{
    if(text == nullptr)
        return;
    uint16_t charWidth = uint16_t(charSize.x);
    uint16_t charHeight = uint16_t(charSize.y);
    uint32_t col = getColor(color.x, color.y, color.z, color.w);

    while(*text)
    {
        const char c = *text++;
        ASSERT(vertData.size() < MAX_LETTERS);

        GPUVertexData vdata;
        vdata.color = col;
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

    addToCopylist(sliceFromPodVectorBytes( vertData ), letterDataBufferHandle[vulk->frameIndex]);
}

void FontRenderSystem::reset()
{
    vertData.clear();
}

void FontRenderSystem::render()
{
    VkCommandBuffer commandBuffer = vulk->commandBuffer;
    if (vertData.size() == 0 || !commandBuffer)
        return;
    beginDebugRegion("Font rendering", Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    beginRenderPass(pipeline, {});

    bindGraphicsPipelineWithDecriptors(pipeline, vulk->frameIndex);
    vkCmdBindIndexBuffer(commandBuffer, letterIndexBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, uint32_t(vertData.size() * 6), 1, 0, 0, 0);

    vkCmdEndRenderPass(vulk->commandBuffer);
    vertData.clear();
    endDebugRegion();
    writeStamp();
}

