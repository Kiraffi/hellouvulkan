#include "convertrendertarget.h"

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>

#include <string>

ConvertRenderTarget::~ConvertRenderTarget()
{
    destroyPipeline(computePipeline);
}

bool ConvertRenderTarget::init(ShaderType shapeType)
{
    // pipelines.
    {
        auto& pipeline = computePipeline;
        std::string name = "Convert pipeline - ";
        name += std::to_string(uint32_t(shapeType));
        if (!createComputePipeline(getShader(shapeType), pipeline, name))
        {
            printf("Failed to create compute pipeline!\n");
            return false;
        }
    }
    return true;
}

bool ConvertRenderTarget::updateSourceImages(const MeshRenderTargets& targets)
{
    return updateSourceImages(targets.normalMapImage, targets.albedoImage);
}

bool ConvertRenderTarget::updateSourceImages(const Image& srcImage, const Image& toImage)
{
    ASSERT(srcImage.width == toImage.width && srcImage.height == toImage.height);

    auto& pipeline = computePipeline;
    destroyDescriptor(pipeline.descriptor);
    
    pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
        {
            DescriptorInfo(vulk->renderFrameBufferHandle),

            DescriptorInfo(srcImage.imageView, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),
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

void ConvertRenderTarget::render(uint32_t width, uint32_t height)
{
    beginDebugRegion("Convert S16ToColor", Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    dispatchCompute(computePipeline, width, height, 1, 8, 8, 1);
    endDebugRegion();
    writeStamp();
}
