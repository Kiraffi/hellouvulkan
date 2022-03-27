#pragma once

#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "core/mytypes.h"
#include "math/vector3.h"

struct GLFWwindow;
constexpr u32 QUERY_COUNT = 128u;

#if USE_RENDERDOC_MARKERS
    const bool useVulkanDebugMarkersRenderDoc = true;
#else
    const bool useVulkanDebugMarkersRenderDoc = false;
#endif

// #define SHOW_INFO_MESSAGES 0

//#define VSYNC 0 // 0 = no vsync, 1 = fifo, 2 = mailbox
//#define DISCRETE_GPU 0

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


/*
struct DeviceWithQueues
{
    VkPhysicalDevice physicalDevice = nullptr;
    VkDevice device = nullptr;
    VkSurfaceKHR surface = nullptr;

    VkCommandPool mainCommandPool = nullptr;
    VkCommandBuffer mainCommandBuffer = nullptr;

    VkQueue graphicsQueue = nullptr;
    VkQueue presentQueue = nullptr;
    VkQueue computeQueue = nullptr;

    QueueFamilyIndices queueFamilyIndices;


    VkFormat computeColorFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkFormat colorFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_MAX_ENUM_KHR;
};
*/
struct Image;

bool initVulkan(GLFWwindow *window);
void deinitVulkan();
bool startRender(GLFWwindow *window);
void present(GLFWwindow *window, Image &presentImage);

//QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
//VkDebugUtilsMessengerEXT registerDebugCallback(VkInstance instance);

//QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
//VkInstance createInstance();
//VkPhysicalDevice createPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
//DeviceWithQueues createDeviceWithQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


void setObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, std::string_view name);
void setObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);
void beginDebugRegion(VkCommandBuffer cmdbuffer, std::string_view pMarkerName, Vec4 color);
void insertDebugRegion(VkCommandBuffer cmdbuffer, std::string_view markerName, Vec4 color);
void endDebugRegion(VkCommandBuffer cmdBuffer);


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

void beginSingleTimeCommands();
void endSingleTimeCommands();

Image createImage(u32 width, u32 height, VkFormat format, VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags, const char *imageName);

void updateImageWithData(u32 width, u32 height, u32 pixelSize,
    Image &targetImage,
    u32 dataSize, void *data);

VkSampler createSampler(const VkSamplerCreateInfo &info);

void destroyImage(Image &image);
void destroySampler(VkSampler sampler);
void destroyShaderModule(VkShaderModule shaderModule);

Buffer createBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char *bufferName);
void destroyBuffer(Buffer &buffer);

size_t uploadToScratchbuffer(void* data, size_t size, size_t offset);
void uploadScratchBufferToGpuBuffer(Buffer &gpuBuffer, size_t sizes);

VkFramebuffer createFramebuffer(VkRenderPass renderPass, VkImageView colorView, VkImageView depthView, u32 width, u32 height);

VkImageView createImageView(VkImage image, VkFormat format);


VkImageMemoryBarrier imageBarrier(Image &image,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT);

VkImageMemoryBarrier imageBarrier(VkImage image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT);

VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, size_t size, size_t offset = 0);
VkBufferMemoryBarrier bufferBarrier(const Buffer &buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);







// Shaders...
struct DescriptorSetLayout
{
    VkDescriptorType descriptorType;
    u32 bindingIndex = ~0u;
};

struct DescriptorSet
{
    VkDescriptorType descriptorType;
    u32 bindingIndex = ~0u;
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

VkPipeline createGraphicsPipeline(VkRenderPass renderPass, VkShaderModule vs, VkShaderModule fs, VkPipelineLayout pipelineLayout,
    bool depthTest);

VkPipeline createComputePipeline(VkShaderModule cs, VkPipelineLayout pipelineLayout);


VkShaderModule loadShader(std::string_view filename);

Pipeline createPipelineLayout(const std::vector<DescriptorSetLayout> &descriptors, VkShaderStageFlags stage);

void destroyPipeline(Pipeline &pipeline);
void destroyDescriptor(Descriptor &descriptor);

Descriptor createDescriptor(const std::vector<DescriptorSetLayout> &descriptors, VkDescriptorSetLayout descriptorSetLayout);
bool setBindDescriptorSet(const std::vector<DescriptorSetLayout> &descriptors,
    const std::vector<DescriptorInfo> &descriptorInfos, VkDescriptorSet descriptorSet);




// Swapchain stuff...

// Intel?
//const VkFormat defaultFormat = VK_FORMAT_R8G8B8A8_UNORM;

#if 0 // HDR
    const VkFormat defaultFormat = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    const VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_HDR10_ST2084_EXT;
#else
    const VkFormat defaultFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM}; //, VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB };
    const VkFormat defaultDepthFormat = VK_FORMAT_D32_SFLOAT;
    const VkColorSpaceKHR defaultColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
#endif

struct GLFWwindow;

struct SwapChain
{
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    std::vector<VkImage> images;

    u32 width = 0;
    u32 height = 0;

    u32 swapchainCount = 0;
};


struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

bool resizeSwapchain(GLFWwindow *window);



struct PipelineWithDescriptors
{
    Pipeline pipeline;

    Descriptor descriptor;
    std::vector<DescriptorSetLayout> descriptorSetLayout;
    std::vector<DescriptorInfo> descriptorSetBinds;
};
void bindPipelineWithDecriptors(VkPipelineBindPoint bindPoint, const PipelineWithDescriptors &pipelineWithDescriptor);









Buffer &getRenderFrameBuffer();
VkRenderPass getRenderPass();


