#pragma once

#include <myvulkan/shader.h>
#include <myvulkan/vulkanglobal.h>
#include <render/meshrendertargets.h>

#include <vulkan/vulkan_core.h>

class ConvertRenderTarget
{
public:
    ConvertRenderTarget(VkFormat fromFormat) : fromFormat(fromFormat) {}
    ~ConvertRenderTarget();


    bool init(ShaderType shapeType);
    bool updateSourceImages(const MeshRenderTargets& targets);
    bool updateSourceImages(const Image& srcImage, const Image& toImage);
    void render(uint32_t width, uint32_t height);
private:
    VkFormat fromFormat = VK_FORMAT_UNDEFINED;
    Pipeline computePipeline;
};
