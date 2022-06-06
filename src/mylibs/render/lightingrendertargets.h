#pragma once

#include <myvulkan/vulkanresources.h>

struct LightingRenderTargets
{
    ~LightingRenderTargets();
    bool resizeLightingTargets();

    void prepareTargetsForLightingComputeWriting();
    void prepareForTonemapSampling();

    Image lightingTargetImage;
};
