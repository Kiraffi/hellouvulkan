#pragma once

#include <string_view>
#include <vulkan/vulkan_core.h>


#include <container/podvector.h>
#include <core/mytypes.h>
#include <math/vector3.h>
#include <myvulkan/vulkglob.h>

#ifdef NDEBUG
    #define VK_CHECK(call) do { [[maybe_unused]] VkResult callResult = call; } while(0)
#else
    #define VK_CHECK(call) do { VkResult callResult = call; ASSERT(callResult == VkResult::VK_SUCCESS); } while(0)
#endif


struct GLFWwindow;
constexpr uint32_t QUERY_COUNT = 128u;
static constexpr uint32_t VulkanApiVersion = VK_API_VERSION_1_2;

bool initVulkan(GLFWwindow *window, const VulkanInitializationParameters &initParameters);
void deinitVulkan();
bool resizeSwapchain(GLFWwindow *window);
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

void destroyShaderModule(VkShaderModule shaderModule);
void destroyPipeline(PipelineWithDescriptors& pipelineWithDescriptors);
void destroyDescriptor(Descriptor &descriptor);

Descriptor createDescriptor(const PodVector<DescriptorSetLayout> &descriptors, VkDescriptorSetLayout descriptorSetLayout);
bool setBindDescriptorSet(const PodVector<DescriptorSetLayout> &descriptors,
    const PodVector<DescriptorInfo> &descriptorInfos, VkDescriptorSet descriptorSet);


void bindPipelineWithDecriptors(VkPipelineBindPoint bindPoint, const PipelineWithDescriptors &pipelineWithDescriptor);

