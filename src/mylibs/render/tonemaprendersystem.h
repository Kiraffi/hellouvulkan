#pragma once

#include <core/mytypes.h>

struct Image;
class TonemapRenderSystem
{
public:
    static bool init();
    static void deinit();
    static bool updateReadTargets(const Image& hdrTexIn, const Image& outputTex);

    static void render(u32 width, u32 height);
};