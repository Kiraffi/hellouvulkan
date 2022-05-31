#pragma once

#include <vulkan/vulkan_core.h>

#include <container/arraysliceview.h>
#include <core/mytypes.h>
#include <myvulkan/uniformbuffermanager.h>
#include <myvulkan/vulkanglobal.h>

struct Buffer;
struct Image;

bool initializeVMA();
void deinitVMA();

Image createImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags, const char* imageName);

void updateImageWithData(uint32_t width, uint32_t height, uint32_t pixelSize,
    Image& targetImage, uint32_t dataSize, void* data);

VkSampler createSampler(const VkSamplerCreateInfo& info);

void destroyImage(Image& image);
void destroySampler(VkSampler sampler);

Buffer createBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char* bufferName);
void destroyBuffer(Buffer& buffer);

size_t uploadToScratchbuffer(void* data, size_t size, size_t offset);
void uploadScratchBufferToGpuBuffer(Buffer& gpuBuffer, size_t sizes);

bool createFramebuffer(Pipeline& pipeline, const PodVector<Image>& colorsAndDepthImages);

void destroyFramebuffer(VkFramebuffer framebuffer);

VkImageView createImageView(VkImage image, VkFormat format);


VkImageMemoryBarrier imageBarrier(Image& image,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

VkImageMemoryBarrier imageBarrier(Image& image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

VkImageMemoryBarrier imageBarrier(VkImage image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, size_t size, size_t offset = 0);
VkBufferMemoryBarrier bufferBarrier(const Buffer& buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);


template <typename T>
bool addToCopylist(const T& objectToCopy, VkBuffer targetBuffer, VkDeviceSize targetOffset)
{
    return addToCopylist(&objectToCopy, VkDeviceSize(sizeof(T)), targetBuffer, targetOffset);
}

template <typename T>
bool addToCopylist(const T& objectToCopy, UniformBufferHandle handle)
{
    return addToCopylist(&objectToCopy, VkDeviceSize(sizeof(T)), handle.manager->buffer->buffer, handle.getOffset());
}

bool addToCopylist(const ArraySliceViewBytes objectToCopy, UniformBufferHandle handle);
bool addToCopylist(const ArraySliceViewBytes objectToCopy, VkBuffer targetBuffer, VkDeviceSize targetOffset);
bool addToCopylist(const ArraySliceViewBytes objectToCopy, Buffer &targetBuffer);

bool addToCopylist(const void *objectToCopy, VkDeviceSize objectSize, VkBuffer targetBuffer, VkDeviceSize targetOffset);
bool addImageBarrier(VkImageMemoryBarrier barrier);
bool flushBarriers(VkPipelineStageFlagBits srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    VkPipelineStageFlagBits dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
