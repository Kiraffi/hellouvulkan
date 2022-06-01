#pragma once

#include <math/vector3.h>

#include <myvulkan/shader.h>

struct Image;

class TonemapRenderSystem
{
public:
    ~TonemapRenderSystem();

    bool init();

    bool updateReadTargets(const Image& hdrTexIn, const Image& outputTex);


    void render(uint32_t width, uint32_t height);


private:
    Pipeline tonemapPipeline;
    VkSampler colorTextureSampler = nullptr;
};