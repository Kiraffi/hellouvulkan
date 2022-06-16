#pragma once

#include <myvulkan/vulkanresources.h>

struct MeshRenderTargets
{
    ~MeshRenderTargets();
    bool resizeMeshTargets();
    bool resizeMeshTargets(uint32_t width, uint32_t height);
    bool resizeShadowTarget(int width, int height);

    void prepareTargetsForMeshRendering();
    void prepareTargetsForShadowRendering();
    void prepareTargetsForLightingComputeSampling();

    Image albedoImage;
    Image normalMapImage;
    Image depthImage;
    Image shadowDepthImage;
};