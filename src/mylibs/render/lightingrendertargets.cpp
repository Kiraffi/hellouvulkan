#include "lightingrendertargets.h"

#include <myvulkan/vulkanresources.h>

LightingRenderTargets::~LightingRenderTargets()
{
    destroyImage(lightingTargetImage);
}

bool LightingRenderTargets::resizeLightingTargets()
{
    uint32_t width = vulk->swapchain.width;
    uint32_t height = vulk->swapchain.height;

    if (!createRenderTargetImage(width, height, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Light HDR color target", lightingTargetImage))
    {
        printf("Failed to create %s\n", lightingTargetImage.imageName);
        return false;
    }

    return true;
}


void LightingRenderTargets::prepareTargetsForLightingComputeWriting()
{
    prepareToComputeImageWrite(lightingTargetImage);
}

void LightingRenderTargets::prepareForTonemapSampling()
{
    prepareToComputeSampleRead(lightingTargetImage);
}