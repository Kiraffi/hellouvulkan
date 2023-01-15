#pragma once

#include <vulkan/vulkan_core.h>

#include <container/arraysliceview.h>
#include <core/mytypes.h>
#include <myvulkan/uniformbuffermanager.h>
#include <myvulkan/vulkanglobal.h>

struct Buffer;
struct Image;

//bool initializeVMA();
//void deinitVMA();

class VulkanResources
{
public:
    static bool init();
    static void update();
    static bool deinit();

    static bool createImage(u32 width, u32 height, VkFormat format, VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryFlags, const char* imageName, Image &outImage);

    static bool createRenderTargetImage(u32 width, u32 height, VkFormat format, VkImageUsageFlags usage,
        const char* imageName, Image& outImage);

    static void updateImageWithData(u32 width, u32 height, u32 pixelSize,
        Image& targetImage, u32 dataSize, void* data);

    static VkSampler createSampler(const VkSamplerCreateInfo& info);

    static void destroyImage(Image& image);
    static void destroySampler(VkSampler sampler);

    static Buffer createBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char* bufferName);
    static void destroyBuffer(Buffer& buffer);

    static size_t uploadToScratchbuffer(void* data, size_t size, size_t offset);
    static void uploadScratchBufferToGpuBuffer(Buffer& gpuBuffer, size_t sizes);

    static bool createFramebuffer(Pipeline& pipeline, const PodVector<Image>& colorsAndDepthImages);

    static void destroyFramebuffer(VkFramebuffer framebuffer);



    static VkImageMemoryBarrier imageBarrier(Image& image,
        VkAccessFlags dstAccessMask, VkImageLayout newLayout);

    static VkImageMemoryBarrier imageBarrier(Image& image,
        VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
        VkAccessFlags dstAccessMask, VkImageLayout newLayout);

    static VkImageMemoryBarrier imageBarrier(VkImage image,
        VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
        VkAccessFlags dstAccessMask, VkImageLayout newLayout,
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

    static VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, size_t size, size_t offset = 0);
    static VkBufferMemoryBarrier bufferBarrier(const Buffer& buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);


    template <typename T>
    static bool addToCopylist(const T& objectToCopy, VkBuffer targetBuffer, VkDeviceSize targetOffset)
    {
        return addToCopylist(
            &objectToCopy, VkDeviceSize(sizeof(T)), targetBuffer, targetOffset);
    }

    template <typename T>
    static bool addToCopylist(const T& objectToCopy, UniformBufferHandle handle)
    {
        if(handle.isValid())
            return addToCopylist(
                &objectToCopy, VkDeviceSize(sizeof(T)),
                handle.manager->buffer->buffer,
                handle.getOffset());
        return false;
    }

    static bool addToCopylist(const ArraySliceViewBytes objectToCopy, UniformBufferHandle handle);
    static bool addToCopylist(const ArraySliceViewBytes objectToCopy, VkBuffer targetBuffer, VkDeviceSize targetOffset);
    static bool addToCopylist(const ArraySliceViewBytes objectToCopy, Buffer &targetBuffer, VkDeviceSize targetOffset = 0u);
    static bool addToCopylist(const void *objectToCopy, VkDeviceSize objectSize, VkBuffer targetBuffer, VkDeviceSize targetOffset);

    static bool addImageComputeBarrier(VkImageMemoryBarrier barrier);
    static bool addImageGraphicsBarrier(VkImageMemoryBarrier barrier);
    static bool flushBarriers(VkPipelineStageFlagBits dstStageMask);// = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);


    static bool prepareToGraphicsSampleWrite(Image& image);
    static bool prepareToGraphicsSampleRead(Image& image);
    static bool prepareToGraphicsImageRead(Image& image);

    static bool prepareToComputeImageWrite(Image& image);
    static bool prepareToComputeImageRead(Image& image);
    static bool prepareToComputeSampleRead(Image& image);
};
