#include "lightrendersystem.h"

#include <container/podvector.h>
#include <container/vector.h>

#include <core/nullable.h>

#include <math/vector3_inline_functions.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

#include <render/lightingrendertargets.h>
#include <render/meshrendertargets.h>

struct LightRenderSystemData
{
    Vec3 m_sunDir{ 0.0f, 1.0f, 0.0f };

    UniformBufferHandle m_lightBufferHandle[VulkanGlobal::FramesInFlight];

    Pipeline m_lightComputePipeline;
    VkSampler m_shadowTextureSampler = VK_NULL_HANDLE;
};

Nullable<LightRenderSystemData> s_lightRenderSystemData;

struct LightBuffer
{
    Vec3 sunDir;
    float lightBufferPadding[16 - 3]{};
};


void LightRenderSystem::deinit()
{
    if(s_lightRenderSystemData.get())
    {
        MyVulkan::destroyPipeline(s_lightRenderSystemData.get()->m_lightComputePipeline);
        VulkanResources::destroySampler(s_lightRenderSystemData.get()->m_shadowTextureSampler);
        s_lightRenderSystemData.destroy();
    }
}

bool LightRenderSystem::init()
{
    s_lightRenderSystemData.create();
    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        s_lightRenderSystemData.get()->m_lightBufferHandle[i] = vulk->uniformBufferManager.reserveHandle();

    auto& pipeline = s_lightRenderSystemData.get()->m_lightComputePipeline;
    pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

    if (!MyVulkan::createComputePipeline(
        VulkanShader::getShader(ShaderType::LightingShader), pipeline, "Light system compute"))
    {
        printf("Failed to create compute pipeline!\n");
        return false;
    }

    {
        VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.borderColor = VkBorderColor::VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        samplerInfo.addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VkCompareOp::VK_COMPARE_OP_LESS;

        s_lightRenderSystemData.get()->m_shadowTextureSampler = VulkanResources::createSampler(samplerInfo);
        if (!s_lightRenderSystemData.get()->m_shadowTextureSampler)
        {
            printf("Failed to create sampler for font rendering");
            return false;
        }
    }
    return true;
}

bool LightRenderSystem::updateReadTargets(const MeshRenderTargets& meshRenderTargets,
    const LightingRenderTargets &lightingRenderTargets)
{
    const auto& outputTex = lightingRenderTargets.lightingTargetImage;
    ASSERT(
        outputTex.width == meshRenderTargets.albedoImage.width    && outputTex.height == meshRenderTargets.albedoImage.height &&
        outputTex.width == meshRenderTargets.normalMapImage.width && outputTex.height == meshRenderTargets.normalMapImage.height &&
        outputTex.width == meshRenderTargets.depthImage.width     && outputTex.height == meshRenderTargets.depthImage.height);

    auto& pipeline = s_lightRenderSystemData.get()->m_lightComputePipeline;

    pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);

    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
    {
        pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
            DescriptorInfo(vulk->renderFrameBufferHandle[i]),
            DescriptorInfo(s_lightRenderSystemData.get()->m_lightBufferHandle[i]),

            //DescriptorInfo(normalTex.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
            //DescriptorInfo(albedoTex.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),

            DescriptorInfo(meshRenderTargets.normalMapImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),
            DescriptorInfo(meshRenderTargets.albedoImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),

            DescriptorInfo(meshRenderTargets.depthImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                s_lightRenderSystemData.get()->m_shadowTextureSampler),
            DescriptorInfo(meshRenderTargets.shadowDepthImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                s_lightRenderSystemData.get()->m_shadowTextureSampler),

            DescriptorInfo(outputTex.imageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE),
        };
    }
    if(!VulkanShader::updateBindDescriptorSet(pipeline))
        return false;

    return true;
}


void LightRenderSystem::update()
{
    if (s_lightRenderSystemData.get()->m_lightBufferHandle[vulk->frameIndex].isValid())
    {
        LightBuffer lightBuffer;
        lightBuffer.sunDir = s_lightRenderSystemData.get()->m_sunDir;

        VulkanResources::addToCopylist(lightBuffer,
            s_lightRenderSystemData.get()->m_lightBufferHandle[vulk->frameIndex]);
    }
}

void LightRenderSystem::render(u32 width, u32 height)
{
    MyVulkan::beginDebugRegion("Lighting compute", Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    MyVulkan::dispatchCompute(s_lightRenderSystemData.get()->m_lightComputePipeline,
        vulk->frameIndex, width, height, 1, 8, 8, 1);
    MyVulkan::endDebugRegion();
    MyVulkan::writeStamp();
}

void LightRenderSystem::setSunDirection(const Vec3& sunDir)
{
    s_lightRenderSystemData.get()->m_sunDir = normalize(sunDir);
}
