
#pragma once


#include <container/podvectorsbase.h>
#include <container/vectorsbase.h>
#include <core/general.h>
#include <core/mytypes.h>
#include <myvulkan/uniformbuffermanager.h>

#include <vulkan/vulkan_core.h>

#ifdef NDEBUG
#define VK_CHECK(call) do { [[maybe_unused]] VkResult callResult = call; } while(0)
#else
#define VK_CHECK(call) do { \
    VkResult callResult = call; \
    if(callResult != VkResult::VK_SUCCESS) \
        printf("Result: %i\n", i32(callResult)); \
    ASSERT(callResult == VkResult::VK_SUCCESS); \
    } while(0)
#endif



struct VmaAllocation_T;
struct VmaAllocator_T;

class VulkanApp;

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
    VkBuffer buffer = 0;
    // cpu mapped memory for cpu accessbuffers
    void *data = nullptr;
    const char *bufferName = nullptr;
    size_t size = 0ull;
    VmaAllocation_T *allocation = nullptr;
};

struct Image
{
    VmaAllocation_T* allocation = nullptr;
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    const char* imageName = nullptr;
    VkAccessFlags accessMask = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    u32 width = 0u;
    u32 height = 0u;
    VkFormat format = VK_FORMAT_UNDEFINED;
};


struct SwapChain
{
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    // Cos Swapchain is in VulkGlob, and VulkGlob instance is static, this podvector gets deleted after main.
    PodVector<VkImage> images;

    u32 width = 0;
    u32 height = 0;

    u32 swapchainCount = 0;
};

struct BufferBarrierInfo
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkBufferCopy copyRegion{};
};

struct VulkanGlobal
{
    static constexpr u32 FramesInFlight = 2;
    static constexpr u32 VulkanMaxScratchBufferFrameSize = 32u * 1024u * 1024u;
    static constexpr u32 VulkanMaxScratchBufferSize = VulkanMaxScratchBufferFrameSize * FramesInFlight;

    QueueFamilyIndices queueFamilyIndices;

    VkDebugUtilsMessengerEXT debugCallBack = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;


    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;


    Buffer scratchBuffer;
    Buffer uniformBuffer;
    u32 scratchBufferOffset = 0u;
    u32 scratchBufferLastFlush = 0u;
    u32 scratchBufferMaxOffset = VulkanMaxScratchBufferSize;
    UniformBufferManager uniformBufferManager;
    UniformBufferHandle renderFrameBufferHandle[FramesInFlight];

    VkQueryPool queryPools[FramesInFlight] = {};
    u32 frameIndex = 0u;
    u32 queryPoolIndexCounts[FramesInFlight] = {};

    VkSemaphore acquireSemaphores[FramesInFlight] = {};
    VkSemaphore releaseSemaphores[FramesInFlight] = {};

    VkFence fences[FramesInFlight] = {};
    VkCommandPool commandPool = VK_NULL_HANDLE;

    VkCommandBuffer commandBuffers[FramesInFlight] = {};
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE; // current commandbuffer


    VmaAllocator_T *allocator = VK_NULL_HANDLE;


    VkSampler globalTextureSampler = VK_NULL_HANDLE;

    VkFormat defaultColorFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkFormat presentColorFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_MAX_ENUM_KHR;

    SwapChain swapchain;

    u32 imageIndex = 0u;

    VulkanApp* vulkanApp = nullptr;
    PodVector< BufferBarrierInfo > bufferMemoryBarriers;
    PodVector< VkBufferMemoryBarrier > bufferMemoryBarriersCopy;
    PodVector< VkImageMemoryBarrier > imageMemoryGraphicsBarriers;
    PodVector< VkImageMemoryBarrier > imageMemoryComputeBarriers;

    VkPipelineStageFlagBits currentStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    bool needToResize = false;
    bool waitForFence = true;
};

extern VulkanGlobal *vulk;





// Shaders...
struct DescriptorSetLayout
{
    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    u32 bindingIndex = ~0u;
    VkPipelineStageFlagBits stage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
};

struct Descriptor
{
    PodVector<VkDescriptorSet> descriptorSets;
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



struct Pipeline
{
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    Descriptor descriptor;
    PodVector<DescriptorSetLayout> descriptorSetLayouts;
    Vector<PodVector<DescriptorInfo>> descriptorSetBinds;

    u32 framebufferWidth = 0u;
    u32 framebufferHeight = 0u;
};


