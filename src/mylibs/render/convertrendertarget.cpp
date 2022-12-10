#include "convertrendertarget.h"

#include <container/podvector.h>
#include <container/string.h>
#include <container/vector.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>

ConvertRenderTarget::~ConvertRenderTarget()
{
    MyVulkan::destroyPipeline(computePipeline);
}

bool ConvertRenderTarget::init(ShaderType shapeType)
{
    // pipelines.
    {
        auto& pipeline = computePipeline;

        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

        String name = "Convert pipeline - ";
        name.append(u32(shapeType));
        if (!MyVulkan::createComputePipeline(
            VulkanShader::getShader(shapeType), pipeline, name.getStr()))
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

    pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);
    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
    {
        pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo> {
            DescriptorInfo(vulk->renderFrameBufferHandle[i]),

            DescriptorInfo(srcImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),
            DescriptorInfo(toImage.imageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE),
        };
    }

    if(!VulkanShader::updateBindDescriptorSet(pipeline))
        return false;

    return true;
}

void ConvertRenderTarget::render(u32 width, u32 height)
{
    MyVulkan::beginDebugRegion("Convert S16ToColor", Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    MyVulkan::dispatchCompute(computePipeline, vulk->frameIndex, width, height, 1, 8, 8, 1);
    MyVulkan::endDebugRegion();
    MyVulkan::writeStamp();
}
