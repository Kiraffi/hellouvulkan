
#pragma once


#include <container/podvectorsbase.h>
#include <container/vectorsbase.h>
#include <core/mytypes.h>
#include <myvulkan/vulkaninitparameters.h>
#include <myvulkan/uniformbuffermanager.h>

#include <vulkan/vulkan_core.h>

#ifdef NDEBUG
#define VK_CHECK(call) do { [[maybe_unused]] VkResult callResult = call; } while(0)
#else
#define VK_CHECK(call) do { \
    VkResult callResult = call; \
    if(callResult != VkResult::VK_SUCCESS) \
        printf("Result: %i\n", int32_t(callResult)); \
    ASSERT(callResult == VkResult::VK_SUCCESS); \
    } while(0)
#endif



struct VmaAllocation_T;
struct VmaAllocator_T;

class VulkanApp;

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
    uint32_t width = 0u;
    uint32_t height = 0u;
    VkFormat format = VK_FORMAT_UNDEFINED;
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

struct BufferBarrierInfo
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkBufferCopy copyRegion{};
};

struct VulkanGlobal
{
    static constexpr uint32_t FramesInFlight = 2;
    static constexpr uint32_t VulkanMaxScratchBufferFrameSize = 32u * 1024u * 1024u;
    static constexpr uint32_t VulkanMaxScratchBufferSize = VulkanMaxScratchBufferFrameSize * FramesInFlight;
    VulkanInitializationParameters initParams;
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
    uint32_t scratchBufferOffset = 0u;
    uint32_t scratchBufferLastFlush = 0u;
    uint32_t scratchBufferMaxOffset = VulkanMaxScratchBufferSize;
    UniformBufferManager uniformBufferManager;
    UniformBufferHandle renderFrameBufferHandle[FramesInFlight];

    VkQueryPool queryPools[FramesInFlight] = {};
    uint32_t frameIndex = 0u;
    uint32_t queryPoolIndexCounts[FramesInFlight] = {};

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

    uint32_t imageIndex = 0u;

    VulkanApp* vulkanApp = nullptr;
    PodVector< BufferBarrierInfo > bufferMemoryBarriers;
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
    uint32_t bindingIndex = ~0u;
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

    uint32_t framebufferWidth = 0u;
    uint32_t framebufferHeight = 0u;
};


