#pragma once

#include <container/podvectorsbase.h>

#include <math/vector3.h>

#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

class LineRenderSystem
{
public:
    ~LineRenderSystem();

    bool init();
    void addLine(const Vec3 &posStart, const Vec3 &posEnd, u32 color);


    void clear();

    bool prepareToRender();

    void render(const Image &colorImage, const Image &depthImage);
    void setRendertargets(const Image &colorImage, const Image &depthImage);
private:
    struct Line
    {
        Vec3 start;
        u32 colorStart;
        Vec3 end;
        u32 colorEnd;
    };

    Buffer vertexBuffer[VulkanGlobal::FramesInFlight];
    Pipeline lineRenderPipeline;

    PodVector<Line> lines;
};