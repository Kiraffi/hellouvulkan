#pragma once

#include <myvulkan/vulkanresources.h>

#include <vulkan/vulkan_core.h>

class VulkanApp;
struct GLFWwindow;

class MyImguiRenderer
{
public:
    ~MyImguiRenderer();
    bool init(GLFWwindow *window);
    // Used with editor only....
    bool updateRenderTarget(const Image &renderTargetImage);
    void renderBegin();
    void render();
    static bool addTexture(const Image &image, VkDescriptorSet &currentId);
private:
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkFramebuffer frameBuffer = VK_NULL_HANDLE;

    uint32_t frameBufferWidth = 0u;
    uint32_t frameBufferHeight = 0u;
};