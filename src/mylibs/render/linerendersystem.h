#pragma once

#include <string_view>

#include <container/podvector.h>

#include <math/vector3.h>

#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

class LineRenderSystem
{
public:
    ~LineRenderSystem();

    bool init();
    void addLine(const Vec3 &posStart, const Vec3 &posEnd, uint32_t color);


    void clear();

    bool prepareToRender();

    void render(const Image &colorImage, const Image &depthImage);
    void setRendertargets(const Image &colorImage, const Image &depthImage);
private:
    struct Line
    {
        Vec3 start;
        uint32_t colorStart;
        Vec3 end;
        uint32_t colorEnd;
    };

    Buffer vertexBuffer[VulkanGlobal::FramesInFlight];
    Pipeline lineRenderPipeline;

    PodVector<Line> lines;
};