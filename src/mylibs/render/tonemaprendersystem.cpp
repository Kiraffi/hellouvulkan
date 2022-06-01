#include "tonemaprendersystem.h"


#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>


TonemapRenderSystem::~TonemapRenderSystem()
{
    destroyPipeline(tonemapPipeline);
}

bool TonemapRenderSystem::init()
{
    auto& pipeline = tonemapPipeline;
    if (!createComputePipeline(getShader(ShaderType::TonemapShader), pipeline, "Tonemap system compute"))
    {
        printf("Failed to create compute pipeline!\n");
        return false;
    }
    return true;
}

bool TonemapRenderSystem::updateReadTargets(const Image& hdrTexIn, const Image& outputTex)
{
    ASSERT(hdrTexIn.width == outputTex.width && hdrTexIn.height == outputTex.height);

    auto& pipeline = tonemapPipeline;
    destroyDescriptor(pipeline.descriptor);

    pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
        {
            DescriptorInfo(vulk->renderFrameBufferHandle),

            DescriptorInfo(hdrTexIn.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
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

void TonemapRenderSystem::render(uint32_t width, uint32_t height)
{
    bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_COMPUTE, tonemapPipeline);
    vkCmdDispatch(vulk->commandBuffer, (width + 7) / 8, (height + 7) / 8, 1);
}
