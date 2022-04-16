
#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>
#include "core/mytypes.h"

struct QueueFamilyIndices
{
    u32 graphicsFamily = ~0u;
    u32 presentFamily = ~0u;
    u32 computeFamily = ~0u;
    u32 transferFamily = ~0u;

    bool isValid()
    {
        return (computeFamily != ~0u && transferFamily != ~0u && graphicsFamily != ~0u && presentFamily != ~0u);
    }
};

struct Formats
{
    VkFormat format;
    VkFormat depth;
    VkColorSpaceKHR colorSpace;
};


// resource
struct Buffer
{
    // cpu mapped memory for cpu accessbuffers
    void *data = nullptr;
    const char *bufferName;
    VkDeviceMemory deviceMemory = 0;
    VkBuffer buffer = 0;
    size_t size = 0ull;
};

struct Image
{
    VkImage image = 0;
    VkImageView imageView = 0;
    VkDeviceMemory deviceMemory = 0;
    const char *imageName;
    VkAccessFlags accessMask = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    u32 width = 0u;
    u32 height = 0u;
};


struct SwapChain
{
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    std::vector<VkImage> images;

    u32 width = 0;
    u32 height = 0;

    u32 swapchainCount = 0;
};


struct VulkGlob
{
    VkDebugUtilsMessengerEXT debugCallBack = nullptr;
    VkInstance instance = nullptr;
    VkPhysicalDevice physicalDevice = nullptr;
    VkDevice device = nullptr;
    VkSurfaceKHR surface = nullptr;

    VkPhysicalDeviceMemoryProperties memoryProperties;

    VkQueue graphicsQueue = nullptr;
    VkQueue presentQueue = nullptr;
    VkQueue computeQueue = nullptr;

    QueueFamilyIndices queueFamilyIndices;

    Buffer scratchBuffer;
    Buffer renderFrameBuffer;

    // Do I need this??
    VkRenderPass renderPass = nullptr;
    VkQueryPool queryPool = nullptr;

    VkSemaphore acquireSemaphore = nullptr;
    VkSemaphore releaseSemaphore = nullptr;

    VkFence fence = nullptr;
    bool waitForFence = true;
    VkCommandPool commandPool = nullptr;

    VkCommandBuffer commandBuffer = nullptr;
    VkFramebuffer targetFB = nullptr;


    VkPipelineCache pipelineCache = nullptr;


    VkFormat computeColorFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkFormat colorFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_MAX_ENUM_KHR;

    SwapChain swapchain;

    Image mainColorRenderTarget;
    Image mainDepthRenderTarget;

    bool needToResize = false;
    uint32_t imageIndex = 0u;
};

extern VulkGlob vulk;