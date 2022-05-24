
#pragma once

#include <vulkan/vulkan_core.h>
#include <core/mytypes.h>

#include <container/podvector.h>
#include "vulkaninitparameters.h"
#include "uniformbuffermanager.h"


#ifdef NDEBUG
#define VK_CHECK(call) do { [[maybe_unused]] VkResult callResult = call; } while(0)
#else
#define VK_CHECK(call) do { VkResult callResult = call; ASSERT(callResult == VkResult::VK_SUCCESS); } while(0)
#endif


struct VmaAllocation_T;
struct VmaAllocator_T;

class VulkanApp;

//#include <vk_mem_alloc.h>

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
    VkBuffer buffer = 0;
    // cpu mapped memory for cpu accessbuffers
    void *data = nullptr;
    const char *bufferName = nullptr;
    /*
    VkDeviceMemory deviceMemory = 0;
    size_t size = 0ull;
    */

    size_t size = 0ull;
    //VmaAllocation allocation = nullptr;
    VmaAllocation_T *allocation = nullptr;
};

struct Image
{
    VkImage image = 0;
    VkImageView imageView = 0;
    const char* imageName = nullptr;
    /*
    VkDeviceMemory deviceMemory = 0;
    */
    VkAccessFlags accessMask = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t width = 0u;
    uint32_t height = 0u;
    VmaAllocation_T *allocation = nullptr;
    //VmaAllocation allocation = nullptr;
};


struct SwapChain
{
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    // Cos Swapchain is in VulkGlob, and VulkGlob instance is static, this podvector gets deleted after main.
    PodVector<VkImage> images;

    uint32_t width = 0;
    uint32_t height = 0;

    uint32_t swapchainCount = 0;
};



struct VulkGlob
{
    VulkanInitializationParameters initParams;
    QueueFamilyIndices queueFamilyIndices;

    VkDebugUtilsMessengerEXT debugCallBack = nullptr;
    VkInstance instance = nullptr;
    VkPhysicalDevice physicalDevice = nullptr;
    VkDevice device = nullptr;
    VkSurfaceKHR surface = nullptr;


    VkQueue graphicsQueue = nullptr;
    VkQueue presentQueue = nullptr;
    VkQueue computeQueue = nullptr;


    Buffer scratchBuffer;
    Buffer uniformBuffer;
    uint32_t scratchBufferOffset = 0u;
    UniformBufferManager uniformBufferManager;
    UniformBufferHandle renderFrameBufferHandle;

    VkQueryPool queryPool = nullptr;

    VkSemaphore acquireSemaphore = nullptr;
    VkSemaphore releaseSemaphore = nullptr;

    VkFence fence = nullptr;
    VkCommandPool commandPool = nullptr;

    VkCommandBuffer commandBuffer = nullptr;
    VmaAllocator_T *allocator = nullptr;


    VkFormat defaultColorFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkFormat presentColorFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_MAX_ENUM_KHR;

    SwapChain swapchain;

    uint32_t imageIndex = 0u;

    VulkanApp* vulkanApp = nullptr;
    PodVector< VkBufferMemoryBarrier > bufferMemoryBarriers;
    PodVector< VkImageMemoryBarrier > imageMemoryBarriers;
    bool needToResize = false;
    bool waitForFence = true;
};

extern VulkGlob vulk;





// Shaders...
struct DescriptorSetLayout
{
    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    uint32_t bindingIndex = ~0u;
    VkPipelineStageFlagBits stage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
};

struct DescriptorSet
{
    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    uint32_t bindingIndex = ~0u;
    const Buffer *buffer = nullptr;
    VkDeviceSize offset = 0;
    VkDeviceSize size = 0;
    VkImage image = 0;
    VkImageView imageView = 0;
    VkSampler sampler = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
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

    DescriptorInfo(const VkImageView imageView, const VkImageLayout layout, const VkSampler sampler)
    {
        imageInfo.imageView = imageView;
        imageInfo.imageLayout = layout;
        imageInfo.sampler = sampler;
        type = DescriptorType::IMAGE;
    }

    //
    DescriptorInfo(const Buffer &buffer, const VkDeviceSize offset, const VkDeviceSize range)
    {
        ASSERT(range > 0u);
        bufferInfo.buffer = buffer.buffer;
        bufferInfo.offset = offset;
        bufferInfo.range = range;
        type = DescriptorType::BUFFER;
    }
    DescriptorInfo(const Buffer &buffer)
    {
        bufferInfo.buffer = buffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = buffer.size;
        type = DescriptorType::BUFFER;
    }

    DescriptorInfo(const UniformBufferHandle handle)
    {
        ASSERT(handle.manager);
        ASSERT(handle.manager->buffer);
        ASSERT(handle.manager->buffer->buffer);
        ASSERT(handle.isValid());

        bufferInfo.buffer = handle.manager->buffer->buffer;
        bufferInfo.offset = handle.getOffset();
        bufferInfo.range = 65536u;
        type = DescriptorType::BUFFER;
    }

    VkDescriptorImageInfo imageInfo {};
    VkDescriptorBufferInfo bufferInfo {};

    DescriptorType type = DescriptorType::NOT_VALID;
};



struct PipelineWithDescriptors
{
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    Descriptor descriptor;
    PodVector<DescriptorSetLayout> descriptorSetLayouts;
    PodVector<DescriptorInfo> descriptorSetBinds;
};


