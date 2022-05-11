#pragma once

#include <string_view>
#include <vulkan/vulkan_core.h>

#include "vulkglob.h"
#include "vulkanresources.h"

#include <container/podvector.h>
#include <core/mytypes.h>

#include <math/vector3.h>


struct GLFWwindow;
constexpr uint32_t QUERY_COUNT = 128u;


struct Image;

bool initVulkan(GLFWwindow *window, const VulkanInitializationParameters &initParameters);
void deinitVulkan();
bool startRender(GLFWwindow *window);
void present(GLFWwindow *window);

void setObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, std::string_view name);
void setObjectTag(uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);
void beginDebugRegion(std::string_view pMarkerName, Vec4 color);
void insertDebugRegion(std::string_view markerName, Vec4 color);
void endDebugRegion();

void beginSingleTimeCommands();
void endSingleTimeCommands();








VkPipeline createGraphicsPipeline(VkRenderPass renderPass, VkShaderModule vs, VkShaderModule fs, VkPipelineLayout pipelineLayout,
    bool depthTest);

VkPipeline createComputePipeline(VkShaderModule cs, VkPipelineLayout pipelineLayout);


VkShaderModule loadShader(std::string_view filename);

bool createPipelineLayout(PipelineWithDescriptors &pipelineWithDescriptors, VkShaderStageFlags stage);

void destroyPipeline(PipelineWithDescriptors& pipelineWithDescriptors);
void destroyDescriptor(Descriptor &descriptor);

Descriptor createDescriptor(const PodVector<DescriptorSetLayout> &descriptors, VkDescriptorSetLayout descriptorSetLayout);
bool setBindDescriptorSet(const PodVector<DescriptorSetLayout> &descriptors,
    const PodVector<DescriptorInfo> &descriptorInfos, VkDescriptorSet descriptorSet);




// Swapchain stuff...
bool resizeSwapchain(GLFWwindow *window);



void bindPipelineWithDecriptors(VkPipelineBindPoint bindPoint, const PipelineWithDescriptors &pipelineWithDescriptor);

