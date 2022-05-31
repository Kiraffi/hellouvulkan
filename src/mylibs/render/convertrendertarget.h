#pragma once

#include <myvulkan/shader.h>
#include <myvulkan/vulkanglobal.h>

#include <vulkan/vulkan_core.h>

class CovertRenderTarget
{
public:
    CovertRenderTarget(VkFormat fromFormat) : fromFormat(fromFormat) {}
    ~CovertRenderTarget();


    bool init(ShaderType shapeType);
    bool updateSourceImage(Image& srcImage, Image& toImage);
    void render(uint32_t width, uint32_t height);
private:
    VkFormat fromFormat = VK_FORMAT_UNDEFINED;
    Pipeline computePipeline;
};
