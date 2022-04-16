#pragma once

#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "core/mytypes.h"
#include "math/vector3.h"

#include "vulkglob.h"

struct GLFWwindow;
constexpr u32 QUERY_COUNT = 128u;

#if USE_RENDERDOC_MARKERS
    const bool useVulkanDebugMarkersRenderDoc = true;
#else
    const bool useVulkanDebugMarkersRenderDoc = false;
#endif

struct Image;

bool initVulkan(GLFWwindow *window);
void deinitVulkan();
bool startRender(GLFWwindow *window);
void present(GLFWwindow *window, Image &presentImage);

void setObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, std::string_view name);
void setObjectTag(uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);
void beginDebugRegion(std::string_view pMarkerName, Vec4 color);
void insertDebugRegion(std::string_view markerName, Vec4 color);
void endDebugRegion();

void beginSingleTimeCommands();
void endSingleTimeCommands();

Image createImage(u32 width, u32 height, VkFormat format, VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags, const char *imageName);

void updateImageWithData(u32 width, u32 height, u32 pixelSize,
    Image &targetImage, u32 dataSize, void *data);

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
bool resizeSwapchain(GLFWwindow *window);



void bindPipelineWithDecriptors(VkPipelineBindPoint bindPoint, const PipelineWithDescriptors &pipelineWithDescriptor);

