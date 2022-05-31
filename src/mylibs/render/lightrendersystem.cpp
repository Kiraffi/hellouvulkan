#include "lightrendersystem.h"


#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>

struct LightBuffer
{
    Vec3 sunDir;
    float lightBufferPadding[16 - 3]{};
};


LightRenderSystem::~LightRenderSystem()
{
    destroyPipeline(lightComputePipeline);
}

bool LightRenderSystem::init()
{
    lightBufferHandle = vulk->uniformBufferManager.reserveHandle();

    auto& pipeline = lightComputePipeline;
    if (!createComputePipeline(getShader(ShaderType::LightingShader), pipeline))
    {
        printf("Failed to create compute pipeline!\n");
        return false;
    }
    return true;
}

bool LightRenderSystem::updateReadTargets(const Image& albedoTex, const Image& normalTex, const Image& outputTex)
{
    ASSERT(albedoTex.width == normalTex.width && albedoTex.height == normalTex.height &&
        albedoTex.width == outputTex.width && albedoTex.height == outputTex.height);

    auto& pipeline = lightComputePipeline;
    destroyDescriptor(pipeline.descriptor);

    pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
        {
            DescriptorInfo(vulk->renderFrameBufferHandle),
            DescriptorInfo(lightBufferHandle),

            DescriptorInfo(normalTex.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
            DescriptorInfo(albedoTex.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
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
    bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_COMPUTE, lightComputePipeline);
    vkCmdDispatch(vulk->commandBuffer, (width + 7) / 8, (height + 7) / 8, 1);
}

void LightRenderSystem::setSunDirection(const Vec3& sunDir)
{
    this->sunDir = normalize(sunDir);
}
