#include "fontrendersystem.h"

#include <app/vulkan_app.h>

#include <container/podvector.h>
#include <container/vector.h>

#include <core/file.h>
#include <core/general.h>
#include <core/mytypes.h>
#include <core/nullable.h>
#include <core/timer.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/uniformbuffermanager.h>
#include <myvulkan/vulkanresources.h>

struct FontRenderSystemData
{
    PodVector<FontRenderSystem::GPUVertexData> m_vertData;

    UniformBufferHandle m_letterDataBufferHandle[VulkanGlobal::FramesInFlight];
    Buffer m_letterIndexBuffer;

    Image m_textImage;
    Pipeline m_pipeline;

    VkSampler m_textureSampler = 0;
};

static Nullable<FontRenderSystemData> s_FontRenderSystemData;

////////////////////////
//
// DEINIT
//
////////////////////////
void FontRenderSystem::deinit()
{
    ASSERT(s_FontRenderSystemData.get());
    if(s_FontRenderSystemData.get())
    {
        VulkanResources::destroyImage(s_FontRenderSystemData.get()->m_textImage);
        MyVulkan::destroyDescriptor(s_FontRenderSystemData.get()->m_pipeline.descriptor);
        MyVulkan::destroyPipeline(s_FontRenderSystemData.get()->m_pipeline);

        VulkanResources::destroySampler(s_FontRenderSystemData.get()->m_textureSampler);

        VulkanResources::destroyBuffer(s_FontRenderSystemData.get()->m_letterIndexBuffer);
    }
    s_FontRenderSystemData.destroy();
}



bool FontRenderSystem::init(const char *fontFilename)
{
    s_FontRenderSystemData.create();
    PodVector<u8> data;
    if (!loadBytes(fontFilename, data.getBuffer()))
    {
        printf("Failed to load file: %s\n", fontFilename);
        return false;
    }

    // Create buffers
    {
        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            s_FontRenderSystemData.get()->m_letterDataBufferHandle[i] =
                vulk->uniformBufferManager.reserveHandle();
        }

        s_FontRenderSystemData.get()->m_letterIndexBuffer = VulkanResources::createBuffer(1 * 1024 * 1024,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index font character buffer");

        {
            u32 offset = 0;
            PodVector<u32> indices;
            indices.uninitializedResize(6 * MAX_LETTERS);
            for (i32 i = 0; i < MAX_LETTERS; ++i)
            {
                indices[size_t(i) * 6 + 0] = i * 4 + 1;
                indices[size_t(i) * 6 + 1] = i * 4 + 0;
                indices[size_t(i) * 6 + 2] = i * 4 + 2;

                indices[size_t(i) * 6 + 3] = i * 4 + 2;
                indices[size_t(i) * 6 + 4] = i * 4 + 0;
                indices[size_t(i) * 6 + 5] = i * 4 + 3;
            }
            offset = VulkanResources::uploadToScratchbuffer(
                (void*)indices.data(),
                size_t(sizeof(indices[0]) * indices.size()),
                offset);
            VulkanResources::uploadScratchBufferToGpuBuffer(
                s_FontRenderSystemData.get()->m_letterIndexBuffer,
                offset);
        }
    }

    //Font image data
    {
        PodVector<u8> fontPic;
        fontPic.resize((128 - 32) * 8 * 12 * 4);

        // Note save order is a bit messed up!!! Since the file has one char 8x12 then next
        u32 index = 0;
        for (i32 y = 0; y < 12; ++y)
        {
            for (i32 charIndex = 0; charIndex < 128 - 32; ++charIndex)
            {
                u8 p = data[y + size_t(charIndex) * 12];
                for (i32 x = 0; x < 8; ++x)
                {
                    u8 bitColor = u8((p >> x) & 1) * 255;
                    fontPic[size_t(index) * 4 + 0] = bitColor;
                    fontPic[size_t(index) * 4 + 1] = bitColor;
                    fontPic[size_t(index) * 4 + 2] = bitColor;
                    fontPic[size_t(index) * 4 + 3] = bitColor;

                    ++index;
                }
            }
        }
        const i32 textureWidth = 8 * (128 - 32);
        const i32 textureHeight = 12;

        if (!VulkanResources::createImage(
            textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            "TextImage", s_FontRenderSystemData.get()->m_textImage))
        {
            printf("Failed to create image for text!\n");
            return false;
        }

        VulkanResources::updateImageWithData(
            textureWidth, textureHeight, 4u,
            s_FontRenderSystemData.get()->m_textImage,
            (u32)fontPic.size(), (void*)fontPic.data());

        VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        s_FontRenderSystemData.get()->m_textureSampler = VulkanResources::createSampler(samplerInfo);
        if (!s_FontRenderSystemData.get()->m_textureSampler)
        {
            printf("Failed to create sampler for font rendering");
            return false;
        }
    }

    // Create pipelines
    {
        auto &pipeline = s_FontRenderSystemData.get()->m_pipeline;
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);
        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);

        pipeline.renderPass = MyVulkan::createRenderPass(
            {
                RenderTarget{ .format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD }
            },
            {}
        );
        ASSERT(pipeline.renderPass);
        if(!pipeline.renderPass)
            return false;

        VkPipelineColorBlendAttachmentState rgbaAtt{ .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        if(!MyVulkan::createGraphicsPipeline(
            VulkanShader::getShader(ShaderType::TexturedQuadVert), VulkanShader::getShader(ShaderType::TexturedQuadFrag),
            { { rgbaAtt } }, {}, pipeline, "Font renderer"))
        {
            printf("failed to create graphics pipeline\n");
            return false;
        }
        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo> {
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),
                DescriptorInfo(s_FontRenderSystemData.get()->m_letterDataBufferHandle[i]),
                DescriptorInfo(s_FontRenderSystemData.get()->m_textImage.imageView,
                    VK_IMAGE_LAYOUT_GENERAL, s_FontRenderSystemData.get()->m_textureSampler),
            };
        }
        if(!VulkanShader::updateBindDescriptorSet(pipeline))
            return false;

    }

    return true;
}

void FontRenderSystem::setRenderTarget(Image& image)
{
    ASSERT(VulkanResources::createFramebuffer(s_FontRenderSystemData.get()->m_pipeline, { image }));
}



void FontRenderSystem::addText(const char *text, Vector2 pos, Vec2 charSize, const Vector4& color)
{
    if(text == nullptr)
        return;
    u16 charWidth = u16(charSize.x);
    u16 charHeight = u16(charSize.y);
    u32 col = getColor(color.x, color.y, color.z, color.w);

    while(*text)
    {
        const char c = *text++;
        ASSERT(s_FontRenderSystemData.get()->m_vertData.size() < MAX_LETTERS);

        GPUVertexData vdata;
        vdata.color = col;
        vdata.pixelSizeX = charWidth;
        vdata.pixelSizeY = charHeight;
        vdata.pos = pos;

        if(c >= 32 && c <= 127)
        {
            u32 letter = c - 32;
            static constexpr float LetterTextureWidth = 128.0f - 32.0f;
            vdata.uvStart = Vec2(float(letter) / LetterTextureWidth, 0.0f);
            vdata.uvSize = Vec2(1.0f / LetterTextureWidth, 1.0f);

            s_FontRenderSystemData.get()->m_vertData.emplace_back(vdata);
        }
        pos.x += charWidth;
    }
}


void FontRenderSystem::update()
{
    if (s_FontRenderSystemData.get()->m_vertData.size() == 0)
        return;

    VulkanResources::addToCopylist(
        sliceFromPodVectorBytes(s_FontRenderSystemData.get()->m_vertData ),
        s_FontRenderSystemData.get()->m_letterDataBufferHandle[vulk->frameIndex]);
}

void FontRenderSystem::reset()
{
    s_FontRenderSystemData.get()->m_vertData.clear();
}

void FontRenderSystem::render()
{
    VkCommandBuffer commandBuffer = vulk->commandBuffer;
    if (s_FontRenderSystemData.get()->m_vertData.size() == 0 || !commandBuffer)
        return;
    MyVulkan::beginDebugRegion("Font rendering", Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    MyVulkan::beginRenderPass(s_FontRenderSystemData.get()->m_pipeline, {});

    MyVulkan::bindGraphicsPipelineWithDescriptors(s_FontRenderSystemData.get()->m_pipeline, vulk->frameIndex);
    vkCmdBindIndexBuffer(
        commandBuffer,
        s_FontRenderSystemData.get()->m_letterIndexBuffer.buffer,
        0,
        VkIndexType::VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, u32(s_FontRenderSystemData.get()->m_vertData.size() * 6), 1, 0, 0, 0);

    vkCmdEndRenderPass(vulk->commandBuffer);
    s_FontRenderSystemData.get()->m_vertData.clear();
    MyVulkan::endDebugRegion();
    MyVulkan::writeStamp();
}

