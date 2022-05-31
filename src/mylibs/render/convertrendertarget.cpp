#include "convertrendertarget.h"

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>

CovertRenderTarget::~CovertRenderTarget()
{
    destroyPipeline(computePipeline);
}

bool CovertRenderTarget::init(ShaderType shapeType)
{
    // pipelines.
    {
        auto& pipeline = computePipeline;
        if (!createComputePipeline(getShader(shapeType), pipeline))
        {
            printf("Failed to create compute pipeline!\n");
            return false;
        }
    }
    return true;
}

bool CovertRenderTarget::updateSourceImage(Image& srcImage, Image& toImage)
{
    ASSERT(srcImage.width == toImage.width && srcImage.height == toImage.height);

    auto& pipeline = computePipeline;
    destroyDescriptor(pipeline.descriptor);
    
    pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
        {
            DescriptorInfo(vulk->renderFrameBufferHandle),

            DescriptorInfo(srcImage.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
            DescriptorInfo(toImage.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
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

void CovertRenderTarget::render(uint32_t width, uint32_t height)
{

    bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdDispatch(vulk->commandBuffer, (width + 7) / 8, (height + 7) / 8, 1);
}
