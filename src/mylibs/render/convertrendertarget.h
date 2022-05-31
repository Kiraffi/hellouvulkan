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
    void render(Image &srcImage, Image& toImage);

private:
    VkFormat fromFormat = VK_FORMAT_UNDEFINED;
    VkShaderModule convertShader = nullptr;
    Pipeline computePipeline;

};
