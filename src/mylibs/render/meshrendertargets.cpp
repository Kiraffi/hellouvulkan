#include "meshrendertargets.h"

#include <myvulkan/vulkanresources.h>

MeshRenderTargets::~MeshRenderTargets()
{
    destroyImage(albedoImage);
    destroyImage(depthImage);
    destroyImage(normalMapImage);
    destroyImage(shadowDepthImage);
}

bool MeshRenderTargets::resizeMeshTargets()
{
    uint32_t width = vulk->swapchain.width;
    uint32_t height = vulk->swapchain.height;

    // create color and depth images
    if (!createRenderTargetImage(width, height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Albedo target image", albedoImage))
    {
        printf("Failed to create %s\n", albedoImage.imageName);
        return false;
    }

    if (!createRenderTargetImage(width, height, VK_FORMAT_R16G16B16A16_SNORM,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, //VK_IMAGE_USAGE_STORAGE_BIT,
        "Normal map target image", normalMapImage))
    {
        printf("Failed to create %s\n", normalMapImage.imageName);
        return false;
    }

    if (!createRenderTargetImage(width, height, vulk->depthFormat,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        "Depth target image", depthImage))
    {
        printf("Failed to create %s\n", depthImage.imageName);
        return false;
    }

    return true;
}

bool MeshRenderTargets::resizeShadowTarget(int width, int height)
{
    if (!createRenderTargetImage(width, height, vulk->depthFormat,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        "Main shadow target", shadowDepthImage))
    {
        printf("Failed to create shadow target image\n");
        return false;
    }
    return true;
}


void MeshRenderTargets::prepareTargetsForMeshRendering()
{
    prepareToGraphicsSampleWrite(albedoImage);
    prepareToGraphicsSampleWrite(normalMapImage);
    prepareToGraphicsSampleWrite(depthImage);
    prepareToGraphicsSampleWrite(shadowDepthImage);
}


void MeshRenderTargets::prepareTargetsForLightingComputeSampling()
{
    prepareToComputeSampleRead(albedoImage);
    prepareToComputeSampleRead(normalMapImage);
    prepareToComputeSampleRead(depthImage);
    prepareToComputeSampleRead(shadowDepthImage);

}
