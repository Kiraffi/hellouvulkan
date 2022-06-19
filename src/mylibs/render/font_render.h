#pragma once

#include <container/podvector.h>
#include <math/vector3.h>

#include <myvulkan/shader.h>
#include <myvulkan/uniformbuffermanager.h>
#include <myvulkan/vulkanresources.h>

static constexpr uint32_t MAX_LETTERS = 10000 * 4;

class FontRenderSystem
{
public:
    FontRenderSystem() {}
    ~FontRenderSystem() {}
    bool init(const char *fontFilename);
    void deInit();
    //void update(VkDevice device, VkCommandBuffer commandBuffer,
    //    VkRenderPass renderPass, Vector2 renderAreaSize, Buffer& scratchBuffer);
    // return offset to scratch buffer
    void update();
    void reset();
    void render();
    void setRenderTarget(Image& image);
    void addText(const char *text, Vector2 pos,
        Vector2 charSize = Vector2(8.0f, 12.0f), const Vector4 &color = Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    struct GPUVertexData
    {
        Vec2 pos;
        uint16_t pixelSizeX;
        uint16_t pixelSizeY;
        uint32_t color;

        Vec2 uvStart;
        Vec2 uvSize;
    };

private:
    PodVector<GPUVertexData> vertData;

    UniformBufferHandle letterDataBufferHandle[VulkanGlobal::FramesInFlight];
    Buffer letterIndexBuffer;

    Image textImage;
    Pipeline pipeline;

    VkSampler textureSampler = 0;
};

