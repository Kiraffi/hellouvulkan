#include "core/timer.h"
#include "core/general.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#include "font_render.h"

#include "core/vulkan_app.h"

#include "core/timer.h"
#include "core/mytypes.h"

#include "myvulkan/myvulkan.h"
//#include "myvulkan/vulkandevice.h"
//#include "myvulkan/vulkanhelperfuncs.h"
//#include "myvulkan/vulkanresource.h"
//#include "myvulkan/vulkanshader.h"
//#include "myvulkan/vulkanswapchain.h"


#include <string>
#include <thread>
#include <vector>
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
    destroyPipeline(pipelinesWithDescriptor.pipeline);

    destroySampler(textureSampler);

    destroyBuffer(letterDataBuffer);
    destroyBuffer(letterIndexBuffer);

    destroyShaderModule(vertexShader);
    destroyShaderModule(fragShader);
}



bool FontRenderSystem::init(std::string_view fontFilename)
{
    std::vector<char> data;
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

        letterDataBuffer = createBuffer(64u * 1024,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Uniform buffer2");

        letterIndexBuffer = createBuffer(1 * 1024 * 1024,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index font character buffer");

        {
            uint32_t offset = 0;
            std::vector<uint32_t> indices;
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
        std::vector<uint8_t> fontPic;
        fontPic.resize((128 - 32) * 8 * 12 * 4);

        // Note save order is a bit messed up!!! Since the file has one char 8x12 then next
        uint32_t index = 0;
        for (int y = 11; y >= 0; --y)
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
        pipeline.descriptorSetLayout = std::vector<DescriptorSetLayout>({
                DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u },
                DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u },
                DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2u }
        });



        pipeline.pipeline = createPipelineLayout(pipeline.descriptorSetLayout, VK_SHADER_STAGE_ALL_GRAPHICS);


        pipeline.pipeline.pipeline = createGraphicsPipeline(vulk.renderPass,
            vertexShader, fragShader,
            pipeline.pipeline.pipelineLayout, false);

        pipeline.descriptorSetBinds = std::vector<DescriptorInfo>(
        {
            DescriptorInfo(vulk.renderFrameBuffer.buffer, 0u, 64u * 1024u ),
            DescriptorInfo(letterDataBuffer.buffer, 0u, 64u * 1024u),
            DescriptorInfo(textImage.imageView, VK_IMAGE_LAYOUT_GENERAL, textureSampler),
        });
        pipeline.descriptor = createDescriptor(pipeline.descriptorSetLayout, pipeline.pipeline.descriptorSetLayout);
        if(!setBindDescriptorSet(pipeline.descriptorSetLayout, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }


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
        vdata.posX = pos.x;
        vdata.posY = pos.y;

        uint32_t letter = c - 32;

        vdata.uvX = float(letter) / float(128-32);
        vdata.uvY = 0.0f;

        vertData.emplace_back(vdata);

        pos.x += charWidth;
    }
}


uint32_t FontRenderSystem::update(Vector2 renderAreaSize, uint32_t offset)
{
    if (vertData.size() == 0)
        return offset;

    VkCommandBuffer commandBuffer = vulk.commandBuffer;
    if(!commandBuffer)
        return offset;

    // use scratch buffer to unifrom buffer transfer
    uint32_t vertDataSize = uint32_t(vertData.size() * sizeof(GPUVertexData));

    memcpy((void*)((char*)vulk.scratchBuffer.data + offset), vertData.data(), vertDataSize);

    VkBufferCopy region = { offset, 0, VkDeviceSize(vertDataSize) };
    vkCmdCopyBuffer(commandBuffer, vulk.scratchBuffer.buffer, letterDataBuffer.buffer, 1, &region);


    VkBufferMemoryBarrier bar[]
    {
        bufferBarrier(letterDataBuffer.buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, vertDataSize)
    };

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, bar, 0, nullptr);

    return offset + vertDataSize;
}



void FontRenderSystem::render()
{
    VkCommandBuffer commandBuffer = vulk.commandBuffer;
    if (vertData.size() > 0 && commandBuffer)
    {
        bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinesWithDescriptor);
        vkCmdBindIndexBuffer(commandBuffer, letterIndexBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, uint32_t(vertData.size() * 6), 1, 0, 0, 0);
    }
    vertData.clear();
}



