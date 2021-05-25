#pragma once

#include <vulkan/vulkan_core.h>

#include <core/mytypes.h>

struct Buffer
{
	// cpu mapped memory for cpu accessbuffers
	void *data = nullptr;
	const char *bufferName;
	VkDeviceMemory deviceMemory = nullptr;
	VkBuffer buffer = nullptr;
	size_t size = 0ull;
};

struct Image
{
	VkImage image = nullptr;
	VkImageView imageView = nullptr;
	VkDeviceMemory deviceMemory = nullptr;
	const char *imageName;
	u32 width = 0u;
	u32 height = 0u;
};

Image createImage(VkDevice device, u32 familyIndex, const VkPhysicalDeviceMemoryProperties &memoryProperties, u32 width, u32 height,
	VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char *imageName);
void destroyImage(VkDevice device, Image &image);

Buffer createBuffer(VkDevice device, const VkPhysicalDeviceMemoryProperties &memoryProperties, size_t size, 
	VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char *bufferName);
void destroyBuffer(VkDevice device, Buffer &buffer);

size_t uploadToScratchbuffer(Buffer& scratchBuffer, void* data, size_t size, size_t offset);
void uploadScratchBufferToGpuBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue, Buffer &gpuBuffer, Buffer &scratchBuffer, size_t sizes);


void uploadBufferToImage(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue, Image &gpuImage, Buffer &scratchBuffer,
						 uint32_t width, uint32_t height, uint32_t bytesPerPixel, uint32_t bufferOffset);
VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass, VkImageView colorView, VkImageView depthView, u32 width, u32 height);

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format);

VkImageMemoryBarrier imageBarrier(VkImage image,
	VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
	VkAccessFlags dstAccessMask, VkImageLayout newLayout,
	VkImageAspectFlags aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT);

VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, size_t size);
VkBufferMemoryBarrier bufferBarrier(const Buffer &buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);
