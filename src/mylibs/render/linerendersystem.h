#pragma once

#include <math/vector3.h>

struct Image;

class LineRenderSystem
{
public:
    static bool init();
    static void deinit();
    static void addLine(const Vec3 &posStart, const Vec3 &posEnd, u32 color);

    static void clear();
    static bool prepareToRender();

    static void render(const Image &colorImage, const Image &depthImage);
    static void setRendertargets(const Image &colorImage, const Image &depthImage);
};