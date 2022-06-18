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
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkFramebuffer frameBuffer = VK_NULL_HANDLE;

    uint32_t frameBufferWidth = 0u;
    uint32_t frameBufferHeight = 0u;
};