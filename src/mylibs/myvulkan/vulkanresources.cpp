#include "vulkanresources.h"

#include <core/mytypes.h>
#include <container/podvector.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanglobal.h>

#include <vk_mem_alloc.h>

//#include <algorithm>
#include <vector>

static std::vector<Image*> globalImages;

static VkImageAspectFlags getAspectMaskFromFormat(VkFormat format)
{
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (format == VK_FORMAT_D32_SFLOAT ||
        format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT ||
        format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
        format == VK_FORMAT_D16_UNORM ||
        format == VK_FORMAT_D16_UNORM_S8_UINT ||
        format == VK_FORMAT_S8_UINT
        )
    {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (format == VK_FORMAT_UNDEFINED)
    {
        ASSERT(!"Undefined format");
        return VK_IMAGE_ASPECT_NONE_KHR;
    }
    return aspectMask;
}

static VkImageView createImageView(VkImage image, VkFormat format)
{
    VkImageAspectFlags aspectMask = getAspectMaskFromFormat(format);

    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;

    //createInfo.components.r = VK_COMPONENT_SWIZZLE_R; // VK_COMPONENT_SWIZZLE_IDENTITY;
    //createInfo.components.g = VK_COMPONENT_SWIZZLE_G; //VK_COMPONENT_SWIZZLE_IDENTITY;
    //createInfo.components.b = VK_COMPONENT_SWIZZLE_B; //VK_COMPONENT_SWIZZLE_IDENTITY;
    //createInfo.components.a = VK_COMPONENT_SWIZZLE_A; //VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView view = 0;
    VK_CHECK(vkCreateImageView(vulk->device, &createInfo, nullptr, &view));

    ASSERT(view);
    return view;
}

bool initVulkanResources()
{
    // init VMA
    {
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion = VulkanApiVersion;
        allocatorCreateInfo.physicalDevice = vulk->physicalDevice;
        allocatorCreateInfo.device = vulk->device;
        allocatorCreateInfo.instance = vulk->instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

        VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &vulk->allocator));
        if (!vulk->allocator)
            return false;


    }
    return true;
}

bool deinitVulkanResources()
{
    if (vulk->device)
    {
        if (vulk->allocator)
            vmaDestroyAllocator(vulk->allocator);
        vulk->allocator = nullptr;
    }
    return true;
}

void vulkanResourceFrameUpdate()
{
    for (auto& image : globalImages)
    {
        image->accessMask = 0u;
        image->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

bool createFramebuffer(Pipeline &pipeline, const PodVector<Image>& colorsAndDepthImages)
{
    destroyFramebuffer(pipeline.framebuffer);
    ASSERT(pipeline.renderPass);
    uint32_t width = 0u;
    uint32_t height = 0u;
    PodVector<VkImageView> views;
    for (const auto& image : colorsAndDepthImages)
    {
        ASSERT(image.imageView);
        if (!image.imageView)
            return false;
        if (width == 0)
        {
            width = image.width;
            height = image.height;
        }
        ASSERT(width == image.width && height == image.height);
        if(width != image.width || height != image.height)
        {
            return false;
        }

        views.push_back(image.imageView);
    }

    ASSERT(width);
    ASSERT(height);

    VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    createInfo.renderPass = pipeline.renderPass;
    createInfo.attachmentCount = views.size();
    createInfo.layers = 1;
    createInfo.pAttachments = views.size() > 0 ? views.data() : nullptr;
    createInfo.width = width;
    createInfo.height = height;

    VkFramebuffer framebuffer = 0;
    VK_CHECK(vkCreateFramebuffer(vulk->device, &createInfo, nullptr, &framebuffer));

    pipeline.framebuffer = framebuffer;
    pipeline.framebufferWidth = width;
    pipeline.framebufferHeight = height;
    ASSERT(framebuffer);
    return framebuffer != VK_NULL_HANDLE;
}

void destroyFramebuffer(VkFramebuffer framebuffer)
{
    if (framebuffer)
        vkDestroyFramebuffer(vulk->device, framebuffer, nullptr);
}

bool createImage(uint32_t width, uint32_t height, VkFormat format,
    VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char* imageName, Image &outImage)
{
    if (outImage.image)
        destroyImage(outImage);

    outImage = Image{};

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
    createInfo.pQueueFamilyIndices = &vulk->queueFamilyIndices.graphicsFamily;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VK_CHECK(vmaCreateImage(vulk->allocator, &createInfo, &allocInfo, &outImage.image, &outImage.allocation, nullptr));
    if (!outImage.image || !outImage.allocation)
        return false;

    outImage.imageView = createImageView(outImage.image, format);
    ASSERT(outImage.imageView);
    if (!outImage.imageView)
        return false;

    setObjectName((uint64_t)outImage.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, imageName);
    outImage.imageName = imageName;
    outImage.width = width;
    outImage.height = height;
    outImage.format = format;
    outImage.layout = createInfo.initialLayout;
    return true;
}

bool createRenderTargetImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
    const char* imageName, Image& outImage)
{
    outImage.imageName = imageName;
    if(width == outImage.width && height == outImage.height && format == outImage.format)
        return true;
    VkImageAspectFlags aspect = getAspectMaskFromFormat(format);

    if (aspect == VK_IMAGE_ASPECT_COLOR_BIT)
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    else if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT)
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    else
    {
        ASSERT(!"Unknown format!\n");
        printf("Trying to create unknown format: %s\n", imageName);
        return false;
    }

    bool success = createImage(width, height, format, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageName, outImage);
    if (!success)
    {
        ASSERT(!"Failed to create render target image!\n");
        printf("Failed to create render target image: %s\n", imageName);
        return false;
    }

    if(std::find(globalImages.begin(), globalImages.end(), &outImage) == globalImages.end())
        globalImages.push_back(&outImage);

    outImage.imageName = imageName;
    return success;
}


Buffer createBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char* bufferName)
{
    Buffer result;

    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = size;
    createInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if(memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocation allocation;
    VmaAllocationInfo vmaAllocInfo;
    VK_CHECK(vmaCreateBuffer(vulk->allocator, &createInfo, &allocInfo, &result.buffer, &allocation, &vmaAllocInfo));

    void* data = nullptr;
    if (memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        data = vmaAllocInfo.pMappedData;
        //VK_CHECK(vmaMapMemory(vulk->allocator, allocation, &data));
//        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
        //VK_CHECK(vkMapMemory(vulk->device, allocation->GetMemory(), 0, size, 0, &data));

        //VK_CHECK(vkMapMemory(vulk->device, result.deviceMemory, 0, size, 0, &data));
        ASSERT(data);
    }
    result.data = data;

    setObjectName((uint64_t)result.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, bufferName);

    result.allocation = allocation;
    result.bufferName = bufferName;
    result.size = size;
    return result;
}


void updateImageWithData(uint32_t width, uint32_t height, uint32_t pixelSize,
    Image& targetImage,
    uint32_t dataSize, void* data)
{

    ASSERT(data != nullptr || dataSize > 0u);
    VkDeviceSize size = vulk->scratchBuffer.size;
    ASSERT(size);

    ASSERT(size >= width * height * pixelSize);
    ASSERT(targetImage.image);

    uint32_t offset = 0u;
    offset = uploadToScratchbuffer(data, dataSize, offset);
    beginSingleTimeCommands();
    {
        VkImageMemoryBarrier imageBarriers[] =
        {
            imageBarrier(targetImage,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
        };

        vkCmdPipelineBarrier(vulk->commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZES(imageBarriers), imageBarriers);
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

    vkCmdCopyBufferToImage(vulk->commandBuffer, vulk->scratchBuffer.buffer, targetImage.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    {
        VkImageMemoryBarrier imageBarriers[] =
        {
            imageBarrier(targetImage,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL),
        };

        vkCmdPipelineBarrier(vulk->commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZES(imageBarriers), imageBarriers);
    }
    endSingleTimeCommands();

    return;
}



VkSampler createSampler(const VkSamplerCreateInfo& info)
{
    VkSampler sampler = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSampler(vulk->device, &info, nullptr, &sampler));
    return sampler;
}

void destroyImage(Image& image)
{
    if(image.imageView)
        vkDestroyImageView(vulk->device, image.imageView, nullptr);
    if(image.image)
        vmaDestroyImage(vulk->allocator, image.image, image.allocation);
    image = Image{};
}


void destroySampler(VkSampler sampler)
{
    if(sampler)
        vkDestroySampler(vulk->device, sampler, nullptr);
}



size_t uploadToScratchbuffer(void* data, size_t size, size_t offset)
{
    ASSERT(vulk->scratchBuffer.data);
    ASSERT(size);

    ASSERT(vulk->scratchBuffer.size >= size);

    Supa::memcpy((unsigned char*)vulk->scratchBuffer.data + offset, data, size);

    size_t roundedDownOffset = offset & (~(size_t(255)));
    size_t roundedUpSize = ((offset + size + 255) & (~(size_t(255)))) - roundedDownOffset;
    vmaFlushAllocation(vulk->allocator, vulk->scratchBuffer.allocation, roundedDownOffset, roundedUpSize);

    offset += size;
    return offset;
}


void uploadScratchBufferToGpuBuffer(Buffer& gpuBuffer, size_t size)
{
    ASSERT(vulk->scratchBuffer.data);
    ASSERT(vulk->scratchBuffer.size >= size);
    ASSERT(gpuBuffer.size >= size);

    beginSingleTimeCommands();

    VkBufferCopy region = { 0, 0, VkDeviceSize(size) };
    vkCmdCopyBuffer(vulk->commandBuffer, vulk->scratchBuffer.buffer, gpuBuffer.buffer, 1, &region);

    VkBufferMemoryBarrier copyBarrier = bufferBarrier(gpuBuffer.buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, size);
    vkCmdPipelineBarrier(vulk->commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyBarrier, 0, nullptr);

    endSingleTimeCommands();

}

void destroyBuffer(Buffer& buffer)
{
    if(!vulk)
        return;
    //if(buffer.data)
    //    vmaUnmapMemory(vulk->allocator, buffer.allocation);
    vmaDestroyBuffer(vulk->allocator, buffer.buffer, buffer.allocation);
    buffer.buffer = 0;
    buffer.data = nullptr;
    buffer.allocation = nullptr;
}






VkImageMemoryBarrier imageBarrier(Image& image,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask = getAspectMaskFromFormat(image.format);
    VkImageMemoryBarrier barrier =
        imageBarrier(image.image, image.accessMask, image.layout, dstAccessMask, newLayout, aspectMask);
    image.accessMask = dstAccessMask;
    image.layout = newLayout;
    return barrier;
}

VkImageMemoryBarrier imageBarrier(Image& image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout)
{
    VkImageAspectFlags aspectMask = getAspectMaskFromFormat(image.format);
    VkImageMemoryBarrier barrier =
        imageBarrier(image.image, srcAccessMask, oldLayout, dstAccessMask, newLayout, aspectMask);
    image.accessMask = dstAccessMask;
    image.layout = newLayout;
    return barrier;
}

VkImageMemoryBarrier imageBarrier(VkImage image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask)
{
    if (srcAccessMask == dstAccessMask && oldLayout == newLayout)
        return VkImageMemoryBarrier{};

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    //barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcQueueFamilyIndex = vulk->queueFamilyIndices.graphicsFamily;
    barrier.dstQueueFamilyIndex = vulk->queueFamilyIndices.graphicsFamily;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
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

VkBufferMemoryBarrier bufferBarrier(const Buffer& buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
{
    return bufferBarrier(buffer.buffer, srcAccessMask, dstAccessMask, buffer.size);
}

bool addToCopylist(const ArraySliceViewBytes objectToCopy, VkBuffer targetBuffer, VkDeviceSize targetOffset)
{
    ASSERT(objectToCopy.isValid());
    if (!objectToCopy.isValid())
        return false;

    uint32_t objectSize = VkDeviceSize(objectToCopy.dataTypeSize) * objectToCopy.size();
    return addToCopylist(objectToCopy.data(), objectSize, targetBuffer, targetOffset);
}

bool addToCopylist(const ArraySliceViewBytes objectToCopy, UniformBufferHandle handle)
{
    Buffer *targetBuffer = handle.manager->buffer;
    if(targetBuffer)
        return addToCopylist(objectToCopy, *targetBuffer, VkDeviceSize(handle.getOffset()));

    return false;
}

bool addToCopylist(const ArraySliceViewBytes objectToCopy, Buffer& targetBuffer, VkDeviceSize targetOffset)
{
    uint32_t objectSize = VkDeviceSize(objectToCopy.dataTypeSize) * objectToCopy.size();
    ASSERT(targetOffset + objectSize <= targetBuffer.size);
    if(targetOffset + objectSize <= targetBuffer.size)
        return addToCopylist(objectToCopy, targetBuffer.buffer, targetOffset);
    return false;
}


bool addToCopylist(const void* objectToCopy, VkDeviceSize objectSize, VkBuffer targetBuffer, VkDeviceSize targetOffset)
{
    ASSERT(objectToCopy);
    ASSERT(objectSize);
    ASSERT(targetBuffer);
    ASSERT(vulk->scratchBufferOffset + objectSize < vulk->scratchBufferMaxOffset);
    if (objectToCopy == nullptr || objectSize == 0 || targetBuffer == VK_NULL_HANDLE ||
        vulk->scratchBufferOffset + objectSize >= vulk->scratchBufferMaxOffset)
        return false;

    if (vulk->imageMemoryGraphicsBarriers.size() == 0 && vulk->imageMemoryComputeBarriers.size() == 0 && vulk->bufferMemoryBarriers.size() == 0)
        beginDebugRegion("Barriers and copies", { 1.0f, 0.0f, 1.0f, 1.0f });

    Supa::memcpy(((uint8_t*)vulk->scratchBuffer.data) + vulk->scratchBufferOffset, objectToCopy, objectSize);

    VkBufferCopy region = { vulk->scratchBufferOffset, targetOffset, objectSize };

    vulk->bufferMemoryBarriers.pushBack({ targetBuffer, region });
    vulk->scratchBufferOffset += objectSize;


    return true;
}

bool addImageGraphicsBarrier(VkImageMemoryBarrier barrier)
{
    if (barrier.sType != VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER)
        return false;
    if(vulk->imageMemoryGraphicsBarriers.size() == 0 && vulk->imageMemoryComputeBarriers.size() && vulk->bufferMemoryBarriers.size() == 0)
        beginDebugRegion("Barriers and copies", { 1.0f, 0.0f, 1.0f, 1.0f });

    vulk->imageMemoryGraphicsBarriers.pushBack(barrier);
    return true;
}

bool addImageComputeBarrier(VkImageMemoryBarrier barrier)
{
    if (barrier.sType != VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER)
        return false;
    if (vulk->imageMemoryGraphicsBarriers.size() == 0 && vulk->imageMemoryComputeBarriers.size() == 0 && vulk->bufferMemoryBarriers.size() == 0)
        beginDebugRegion("Barriers and copies", { 1.0f, 0.0f, 1.0f, 1.0f });

    vulk->imageMemoryComputeBarriers.pushBack(barrier);
    return true;
}


bool flushBarriers(VkPipelineStageFlagBits dstStageMask)
{
    bool isGraphics = dstStageMask == VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    bool isCompute = dstStageMask == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    const VkImageMemoryBarrier* imageBarrier = nullptr;
    uint32_t imageBarrierCount = 0u;
    if (isGraphics)
    {
        imageBarrierCount = vulk->imageMemoryGraphicsBarriers.size();
        imageBarrier = imageBarrierCount > 0 ? vulk->imageMemoryGraphicsBarriers.data() : nullptr;
    }
    else if (isCompute)
    {
        imageBarrierCount = vulk->imageMemoryComputeBarriers.size();
        imageBarrier = imageBarrierCount > 0 ? vulk->imageMemoryComputeBarriers.data() : nullptr;
    }

    if (vulk->bufferMemoryBarriers.size() == 0 && imageBarrierCount == 0)
        return true;
   // vkFlushMappedMemoryRanges?????
    //PodVector< VkBufferMemoryBarrier > bufferBarriers;

    // for flushing
    VkDeviceSize size = vulk->scratchBufferOffset - vulk->scratchBufferLastFlush;
    size = ((size + 255) / 256) * 256;

    VK_CHECK(vmaFlushAllocation(vulk->allocator, vulk->scratchBuffer.allocation, vulk->scratchBufferLastFlush, size));
    // set the scratchBufferLastFlush to be divisable by 256 and round it down
    vulk->scratchBufferLastFlush = vulk->scratchBufferOffset & (~uint32_t(255));

    for (const auto& barrier : vulk->bufferMemoryBarriers)
    {
        if (barrier.buffer)
        {
            vkCmdCopyBuffer(vulk->commandBuffer, vulk->scratchBuffer.buffer, barrier.buffer, 1, &barrier.copyRegion);
            vulk->bufferMemoryBarriersCopy.push_back(bufferBarrier(barrier.buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
                barrier.copyRegion.size, barrier.copyRegion.dstOffset));
        }
    }

    const VkBufferMemoryBarrier* bufferBarrier = vulk->bufferMemoryBarriersCopy.size() > 0 ? vulk->bufferMemoryBarriersCopy.data() : nullptr;

    vkCmdPipelineBarrier(vulk->commandBuffer, vulk->currentStage, dstStageMask,
        VK_DEPENDENCY_BY_REGION_BIT,
        0, nullptr,
        vulk->bufferMemoryBarriersCopy.size(), bufferBarrier,
        imageBarrierCount, imageBarrier);

    vulk->bufferMemoryBarriers.clear();
    vulk->bufferMemoryBarriersCopy.clear();
    if(isGraphics)
        vulk->imageMemoryGraphicsBarriers.clear();
    if (isCompute)
        vulk->imageMemoryComputeBarriers.clear();
    endDebugRegion();

    vulk->currentStage = dstStageMask;
    return true;
}

bool prepareToGraphicsSampleWrite(Image& image)
{
    VkImageAspectFlags aspectMask = getAspectMaskFromFormat(image.format);
    if (aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
    {
        return addImageGraphicsBarrier(imageBarrier(image,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    }
    else if (aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT)
    {
        return addImageGraphicsBarrier(imageBarrier(image,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
    }
    else
    {
        ASSERT(!"Unknown imagetype");
        return false;
    }
    return true;
}

bool prepareToGraphicsSampleRead(Image& image)
{
    return addImageGraphicsBarrier(imageBarrier(image,
        VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
}

bool prepareToGraphicsImageRead(Image& image)
{
    return addImageGraphicsBarrier(imageBarrier(image,
        VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));
}

bool prepareToComputeImageWrite(Image& image)
{
    return addImageComputeBarrier(imageBarrier(image,
        VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL));
}

bool prepareToComputeImageRead(Image& image)
{
    return addImageComputeBarrier(imageBarrier(image,
        VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));
}

bool prepareToComputeSampleRead(Image& image)
{
    return addImageComputeBarrier(imageBarrier(image,
        VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
}
