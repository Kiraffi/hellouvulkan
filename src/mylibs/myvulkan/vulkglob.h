
#pragma once

#include <vulkan/vulkan_core.h>
#include "core/mytypes.h"
#include "container/podvector.h"

struct QueueFamilyIndices
{
    uint32_t graphicsFamily = ~0u;
    uint32_t presentFamily = ~0u;
    uint32_t computeFamily = ~0u;
    uint32_t transferFamily = ~0u;

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
    uint32_t width = 0u;
    uint32_t height = 0u;
};


struct SwapChain
{
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    PodVector<VkImage> images;

    uint32_t width = 0;
    uint32_t height = 0;

    uint32_t swapchainCount = 0;
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





// Shaders...
struct DescriptorSetLayout
{
    VkDescriptorType descriptorType;
    uint32_t bindingIndex = ~0u;
};

struct DescriptorSet
{
    VkDescriptorType descriptorType;
    uint32_t bindingIndex = ~0u;
    const Buffer *buffer = nullptr;
    VkDeviceSize offset = 0;
    VkDeviceSize size = 0;
    VkImage image = 0;
    VkImageView imageView = 0;
    VkSampler sampler = 0;
    VkImageLayout layout;
};

struct Descriptor
{
    VkDescriptorSet descriptorSet = 0;
    VkDescriptorPool pool = 0;
};

struct DescriptorInfo
{
    enum class DescriptorType
    {
        IMAGE,
        BUFFER,

        NOT_VALID
    };

    DescriptorInfo(VkImageView imageView, VkImageLayout layout, VkSampler sampler)
    {
        imageInfo.imageView = imageView;
        imageInfo.imageLayout = layout;
        imageInfo.sampler = sampler;
        type = DescriptorType::IMAGE;
    }

    //
    DescriptorInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
    {
        ASSERT(range > 0u);
        bufferInfo.buffer = buffer;
        bufferInfo.offset = offset;
        bufferInfo.range = range;
        type = DescriptorType::BUFFER;
    }

    VkDescriptorImageInfo imageInfo {};
    VkDescriptorBufferInfo bufferInfo {};

    DescriptorType type = DescriptorType::NOT_VALID;
};



struct Pipeline
{
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
};

struct PipelineWithDescriptors
{
    Pipeline pipeline;

    Descriptor descriptor;
    PodVector<DescriptorSetLayout> descriptorSetLayout;
    PodVector<DescriptorInfo> descriptorSetBinds;
};

