#include "lightrendersystem.h"


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
    lightBufferHandle = vulk->uniformBufferManager.reserveHandle();

    auto& pipeline = lightComputePipeline;
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

bool LightRenderSystem::updateReadTargets(const Image& albedoTex, const Image& normalTex, const Image& depthTex,
    const Image& shadowTex, const Image& outputTex)
{
    ASSERT(albedoTex.width == normalTex.width && albedoTex.height == normalTex.height &&
        albedoTex.width == depthTex.width && albedoTex.height == depthTex.height &&
        albedoTex.width == outputTex.width && albedoTex.height == outputTex.height);

    auto& pipeline = lightComputePipeline;
    destroyDescriptor(pipeline.descriptor);

    pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
        {
            DescriptorInfo(vulk->renderFrameBufferHandle),
            DescriptorInfo(lightBufferHandle),

            //DescriptorInfo(normalTex.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
            //DescriptorInfo(albedoTex.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),

            DescriptorInfo(normalTex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),
            DescriptorInfo(albedoTex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),

            DescriptorInfo(depthTex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shadowTextureSampler),
            DescriptorInfo(shadowTex.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shadowTextureSampler),

            DescriptorInfo(outputTex.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
        });
    if (!createDescriptor(pipeline))
    {
        printf("Failed to create compute pipeline descriptor\n");
        return false;
    }
    if (!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
    {
        printf("Failed to set descriptor binds!\n");
        return false;
    }
    return true;
}


void LightRenderSystem::update()
{
    if (lightBufferHandle.isValid())
    {
        LightBuffer buffer;
        buffer.sunDir = sunDir;

        addToCopylist(buffer, lightBufferHandle);
    }
}

void LightRenderSystem::render(uint32_t width, uint32_t height)
{
    beginDebugRegion("Lighting compute", Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    
    bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_COMPUTE, lightComputePipeline);
    vkCmdDispatch(vulk->commandBuffer, (width + 7) / 8, (height + 7) / 8, 1);

    endDebugRegion();
}

void LightRenderSystem::setSunDirection(const Vec3& sunDir)
{
    this->sunDir = normalize(sunDir);
}
