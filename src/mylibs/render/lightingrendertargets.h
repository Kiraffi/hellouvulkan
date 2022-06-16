#pragma once

#include <myvulkan/vulkanresources.h>

struct LightingRenderTargets
{
    ~LightingRenderTargets();
    bool resizeLightingTargets();
    bool resizeLightingTargets(uint32_t width, uint32_t height);

    void prepareTargetsForLightingComputeWriting();
    void prepareForTonemapSampling();

    Image lightingTargetImage;
};
