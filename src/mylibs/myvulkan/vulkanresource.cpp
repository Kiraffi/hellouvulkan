#include "vulkanresource.h"

#include <stdio.h>
#include <memory.h>

#include "vulkandevice.h"
#include "core/mytypes.h"


static u32 selectMemoryType(const VkPhysicalDeviceMemoryProperties &memoryProperties, u32 memoryTypeBits, VkMemoryPropertyFlags flags)
{
	for(u32 i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		bool isUsed = (memoryTypeBits & (1 << i)) != 0u;
		u32 foundFlags = (memoryProperties.memoryTypes[i].propertyFlags & flags);

		//printf("Memory %u: Used: %u, flags: %u\n", i, isUsed, foundFlags);

		if(isUsed && foundFlags == flags)
			return i;
	}
//1, 3, 6,7, 10, 11
	ASSERT(!"No compatible memory found!");
	return ~0u;
}








void beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
{
	VK_CHECK(vkResetCommandPool(device, commandPool, 0));

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	return;
}

void endSingleTimeCommands(VkDevice device, VkCommandBuffer commandBuffer, VkQueue queue)
{
	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(queue));
}








Image createImage(VkDevice device, u32 familyIndex, const VkPhysicalDeviceMemoryProperties &memoryProperties, u32 width, u32 height, VkFormat format,
	VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char *imageName)
{

	VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = format;
	createInfo.extent = { width, height, 1 };
	createInfo.mipLevels = 1;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = usage;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.queueFamilyIndexCount = 1;
	createInfo.pQueueFamilyIndices = &familyIndex;
	Image result;

	VK_CHECK(vkCreateImage(device, &createInfo, nullptr, &result.image));
	ASSERT(result.image);

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, result.image, &memoryRequirements);

	u32 memoryTypeIndex = selectMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags);
	ASSERT(memoryTypeIndex != ~0u);

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;

	VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &result.deviceMemory));
	ASSERT(result.deviceMemory);

	VK_CHECK(vkBindImageMemory(device, result.image, result.deviceMemory, 0));

	result.imageView = createImageView(device, result.image, format);
	ASSERT(result.imageView);

/*
	// If cpu image access?
	void *data = nullptr;
	if(memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		VK_CHECK(vkMapMemory(device, result.deviceMemory, 0, size, 0, &data));
		assert(data);
	}
	result.data = data;
	result.size = size;
*/
	setObjectName(device, (uint64_t)result.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, imageName);
	result.imageName = imageName;

	return result;
}



void updateImageWithData(VkDevice device, VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue,
	u32 width, u32 height, u32 pixelSize,
	Buffer& scratchBuffer, Image &targetImage,
	u32 dataSize, void *data)
{
	ASSERT(data != nullptr || dataSize > 0u);
	ASSERT(scratchBuffer.data);
	ASSERT(scratchBuffer.size >= width * height * pixelSize);
	ASSERT(targetImage.image);

	uint32_t offset = 0u;
	offset = uploadToScratchbuffer(scratchBuffer, data, dataSize, offset);
	beginSingleTimeCommands(device, commandPool, commandBuffer);
	{
		VkImageMemoryBarrier imageBarriers[] =
		{
			imageBarrier(targetImage,
						VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
		};

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(imageBarriers), imageBarriers);
	}

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, scratchBuffer.buffer,	targetImage.image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	{
		VkImageMemoryBarrier imageBarriers[] =
		{
			imageBarrier(targetImage,
						VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL),
		};

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(imageBarriers), imageBarriers);
	}
	endSingleTimeCommands(device, commandBuffer, queue);

	return;
}

void destroyImage(VkDevice device, Image &image)
{
	vkDestroyImageView(device, image.imageView, nullptr);
	vkDestroyImage(device, image.image, nullptr);
	vkFreeMemory(device, image.deviceMemory, nullptr);

	image = {};
}

Buffer createBuffer(VkDevice device, const VkPhysicalDeviceMemoryProperties &memoryProperties, size_t size,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char *bufferName)
{
	Buffer result;

	VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = size;
	createInfo.usage = usage;

	VK_CHECK(vkCreateBuffer(device, &createInfo, nullptr, &result.buffer));
	ASSERT(result.buffer);

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, result.buffer, &memoryRequirements);

	u32 memoryTypeIndex = selectMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags);
	ASSERT(memoryTypeIndex != ~0u);

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;

	VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &result.deviceMemory));
	ASSERT(result.deviceMemory);

	VK_CHECK(vkBindBufferMemory(device, result.buffer, result.deviceMemory, 0));

	void *data = nullptr;
	if(memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		VK_CHECK(vkMapMemory(device, result.deviceMemory, 0, size, 0, &data));
		ASSERT(data);
	}
	result.data = data;
	result.size = size;


	setObjectName(device, (uint64_t)result.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, bufferName);
	result.bufferName = bufferName;

	return result;
}

size_t uploadToScratchbuffer(Buffer& scratchBuffer, void* data, size_t size, size_t offset)
{
	ASSERT(scratchBuffer.data);
	ASSERT(size);
	ASSERT(scratchBuffer.size >= size);

	memcpy((unsigned char*)scratchBuffer.data + offset, data, size);
	offset += size;

	return offset;
}

void uploadBufferToImage(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue, Image &gpuImage, Buffer &scratchBuffer, 
						 uint32_t width, uint32_t height, uint32_t bytesPerPixel, uint32_t bufferOffset)
{

	
	//	endSingleTimeCommands(device, commandBuffer, queue);
	
}



void uploadScratchBufferToGpuBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue, Buffer &gpuBuffer, Buffer &scratchBuffer, size_t size)
{
	ASSERT(scratchBuffer.data);
	ASSERT(scratchBuffer.size >= size);
	ASSERT(gpuBuffer.size >= size);

	beginSingleTimeCommands(device, commandPool, commandBuffer);

	VkBufferCopy region = { 0, 0, VkDeviceSize(size) };
	vkCmdCopyBuffer(commandBuffer, scratchBuffer.buffer, gpuBuffer.buffer, 1, &region);

	VkBufferMemoryBarrier copyBarrier = bufferBarrier(gpuBuffer.buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, size);
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyBarrier, 0, nullptr);

	endSingleTimeCommands(device, commandBuffer, queue);

}

void destroyBuffer(VkDevice device, Buffer &buffer)
{
	vkDestroyBuffer(device, buffer.buffer, nullptr);
	vkFreeMemory(device, buffer.deviceMemory, nullptr);
	buffer.buffer = 0;
	buffer.data = nullptr;
	buffer.deviceMemory = 0;
	buffer.size = 0ll;
}


VkFramebuffer createFramebuffer(VkDevice device, VkRenderPass renderPass,
	VkImageView colorView, VkImageView depthView, u32 width, u32 height)
{
	VkImageView attachMents[] = { colorView, depthView };
	VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	createInfo.renderPass = renderPass;
	createInfo.attachmentCount = 2;
	createInfo.layers = 1;
	createInfo.pAttachments = attachMents;
	createInfo.width = width;
	createInfo.height = height;

	VkFramebuffer framebuffer = 0;
	VK_CHECK(vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffer));
	return framebuffer;
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format)
{
	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	
	if(format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
		format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D16_UNORM_S8_UINT ||
		format == VK_FORMAT_S8_UINT
	)
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

	VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	
	createInfo.components.r = VK_COMPONENT_SWIZZLE_R; // VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_G; //VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_B; //VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_A; //VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = aspectMask;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;


	VkImageView view = 0;
	VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &view));

	ASSERT(view);
	return view;
}


VkImageMemoryBarrier imageBarrier(Image &image,
	VkAccessFlags dstAccessMask, VkImageLayout newLayout,
	VkImageAspectFlags aspectMask)
{
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = image.accessMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = image.layout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image.image;
	barrier.subresourceRange.aspectMask = aspectMask;
	// Andoird error?
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	

	image.accessMask = dstAccessMask;
	image.layout = newLayout;
	return barrier;
}


VkImageMemoryBarrier imageBarrier(VkImage image,
	VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
	VkAccessFlags dstAccessMask, VkImageLayout newLayout,
	VkImageAspectFlags aspectMask)
{
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspectMask;
	// Andoird error?
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	
	return barrier;
}

VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, size_t size, size_t offset)
{
	ASSERT(size > 0);
	VkBufferMemoryBarrier copyBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	copyBarrier.srcAccessMask = srcAccessMask;
	copyBarrier.dstAccessMask = dstAccessMask;
	copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	copyBarrier.buffer = buffer;
	copyBarrier.offset = offset;
	copyBarrier.size = size;
	return copyBarrier;
}

VkBufferMemoryBarrier bufferBarrier(const Buffer &buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
{
	return bufferBarrier(buffer.buffer, srcAccessMask, dstAccessMask, buffer.size);
}

