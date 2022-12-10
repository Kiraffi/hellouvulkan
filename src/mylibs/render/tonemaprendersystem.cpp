#include "tonemaprendersystem.h"

#include <container/podvector.h>
#include <container/vector.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>


TonemapRenderSystem::~TonemapRenderSystem()
{
    MyVulkan::destroyPipeline(tonemapPipeline);
}

bool TonemapRenderSystem::init()
{
    auto& pipeline = tonemapPipeline;
    pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

    if (!MyVulkan::createComputePipeline(
            VulkanShader::getShader(ShaderType::TonemapShader),
            pipeline, "Tonemap system compute"))
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
    pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);
    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
    {
        pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
            DescriptorInfo(vulk->renderFrameBufferHandle[i]),

            //DescriptorInfo(hdrTexIn.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),

            DescriptorInfo(hdrTexIn.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),
            DescriptorInfo(outputTex.imageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE),


        };
    }

    if (!VulkanShader::updateBindDescriptorSet(pipeline))
        return false;

    return true;
}

void TonemapRenderSystem::render(u32 width, u32 height)
{
    MyVulkan::beginDebugRegion("Tonemap", Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    MyVulkan::dispatchCompute(tonemapPipeline, vulk->frameIndex, width, height, 1, 8, 8, 1);
    MyVulkan::endDebugRegion();
    MyVulkan::writeStamp();
}
