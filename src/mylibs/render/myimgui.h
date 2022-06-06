#pragma once

#include <vulkan/vulkan_core.h>

class VulkanApp;
struct GLFWwindow;
struct Image;

class MyImgui
{
public:
    ~MyImgui();
    bool init(GLFWwindow* window);
    bool updateRenderTarget(const Image &image);
    void renderBegin();
    void render();

private:
    VkRenderPass renderPass = nullptr;
    VkDescriptorPool descriptorPool = nullptr;
    VkFramebuffer frameBuffer = nullptr;

    uint32_t frameBufferWidth = 0u;
    uint32_t frameBufferHeight = 0u;
};