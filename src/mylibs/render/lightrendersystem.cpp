#include "lightrendersystem.h"

#include <container/podvector.h>
#include <container/vector.h>

#include <math/vector3_inline_functions.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>

struct LightBuffer
{
    Vec3 sunDir;
    float lightBufferPadding[16 - 3]{};
};


LightRenderSystem::~LightRenderSystem()
{
    destroyPipeline(lightComputePipeline);
    destroySampler(shadowTextureSampler);
}

bool LightRenderSystem::init()
{
    for(uint32_t i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        lightBufferHandle[i] = vulk->uniformBufferManager.reserveHandle();

    auto& pipeline = lightComputePipeline;
    pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

    if (!createComputePipeline(getShader(ShaderType::LightingShader), pipeline, "Light system compute"))
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

        shadowTextureSampler = createSampler(samplerInfo);
        if (!shadowTextureSampler)
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

    auto& pipeline = lightComputePipeline;

    pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);

    for(uint32_t i = 0; i < VulkanGlobal::FramesInFlight; ++i)
    {
        pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
            DescriptorInfo(vulk->renderFrameBufferHandle[i]),
            DescriptorInfo(lightBufferHandle[i]),

            //DescriptorInfo(normalTex.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
            //DescriptorInfo(albedoTex.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),

            DescriptorInfo(meshRenderTargets.normalMapImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),
            DescriptorInfo(meshRenderTargets.albedoImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),

            DescriptorInfo(meshRenderTargets.depthImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shadowTextureSampler),
            DescriptorInfo(meshRenderTargets.shadowDepthImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shadowTextureSampler),

            DescriptorInfo(outputTex.imageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE),
        };
    }
    if(!updateBindDescriptorSet(pipeline))
        return false;

    return true;
}


void LightRenderSystem::update()
{
    if (lightBufferHandle[vulk->frameIndex].isValid())
    {
        LightBuffer buffer;
        buffer.sunDir = sunDir;

        addToCopylist(buffer, lightBufferHandle[vulk->frameIndex]);
    }
}

void LightRenderSystem::render(uint32_t width, uint32_t height)
{
    beginDebugRegion("Lighting compute", Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    dispatchCompute(lightComputePipeline, vulk->frameIndex, width, height, 1, 8, 8, 1);
    endDebugRegion();
    writeStamp();
}

void LightRenderSystem::setSunDirection(const Vec3& sunDir)
{
    this->sunDir = normalize(sunDir);
}
