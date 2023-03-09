#pragma once

#include <container/podvector.h>
#include <math/vector3.h>

struct Image;

static constexpr u32 MAX_LETTERS = 10000 * 4;

class RenderSystem
{
public:
    RenderSystem();
    virtual ~RenderSystem();

    void virtual updateRenderTargets(PodVector<Image> &images);
    void virtual update();
    void virtual reset();
};

enum RenderResolutions
{
    FULL_WINDOW_SIZE,
    RENDER_RESOLUTION_FULL,
    RENDER_RESOLUTION_QUARTER,

};

class RenderTargets
{
public:

private:
    f32 width;
    f32 height;
};

class FontRenderSystem
{
public:
    static bool init(const char *fontFilename);
    static void deinit();
    //static //void update(VkDevice device, VkCommandBuffer commandBuffer,
    //static //    VkRenderPass renderPass, Vector2 renderAreaSize, Buffer& scratchBuffer);
    //static // return offset to scratch buffer
    static void update();
    static void reset();
    static void render();
    static void setRenderTarget(Image& image);
    static void addText(const char *text, Vector2 pos,
        Vector2 charSize = Vector2(8.0f, 12.0f), const Vector4 &color = Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    struct GPUVertexData
    {
        Vec2 pos;
        u16 pixelSizeX;
        u16 pixelSizeY;
        u32 color;

        Vec2 uvStart;
        Vec2 uvSize;
    };
};

