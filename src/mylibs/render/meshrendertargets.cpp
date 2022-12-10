#include "meshrendertargets.h"

#include <myvulkan/vulkanresources.h>

MeshRenderTargets::~MeshRenderTargets()
{
    VulkanResources::destroyImage(albedoImage);
    VulkanResources::destroyImage(depthImage);
    VulkanResources::destroyImage(normalMapImage);
    VulkanResources::destroyImage(shadowDepthImage);
}

bool MeshRenderTargets::resizeMeshTargets()
{
    uint32_t width = vulk->swapchain.width;
    uint32_t height = vulk->swapchain.height;
    return resizeMeshTargets(width, height);
}

bool MeshRenderTargets::resizeMeshTargets(uint32_t width, uint32_t height)
{
    // create color and depth images
    if (!VulkanResources::createRenderTargetImage(width, height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Albedo target image", albedoImage))
    {
        printf("Failed to create %s\n", albedoImage.imageName);
        return false;
    }

    if (!VulkanResources::createRenderTargetImage(width, height, VK_FORMAT_R16G16B16A16_SNORM,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, //VK_IMAGE_USAGE_STORAGE_BIT,
        "Normal map target image", normalMapImage))
    {
        printf("Failed to create %s\n", normalMapImage.imageName);
        return false;
    }

    if (!VulkanResources::createRenderTargetImage(width, height, vulk->depthFormat,
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
    if (!VulkanResources::createRenderTargetImage(width, height, vulk->depthFormat,
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
    VulkanResources::prepareToGraphicsSampleWrite(albedoImage);
    VulkanResources::prepareToGraphicsSampleWrite(normalMapImage);
    VulkanResources::prepareToGraphicsSampleWrite(depthImage);
}

void MeshRenderTargets::prepareTargetsForShadowRendering()
{
    VulkanResources::prepareToGraphicsSampleWrite(shadowDepthImage);
}


void MeshRenderTargets::prepareTargetsForLightingComputeSampling()
{
    VulkanResources::prepareToComputeSampleRead(albedoImage);
    VulkanResources::prepareToComputeSampleRead(normalMapImage);
    VulkanResources::prepareToComputeSampleRead(depthImage);
    VulkanResources::prepareToComputeSampleRead(shadowDepthImage);
}
