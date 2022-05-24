#pragma once

#include <string_view>
#include <vulkan/vulkan_core.h>

#include <container/arraysliceview.h>
#include <core/mytypes.h>
#include <math/vector3.h>
#include <myvulkan/vulkglob.h>


class VulkanApp;
struct Shader;

struct DepthTest
{
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    bool useDepthTest = false;
    bool writeDepth = false;
};

struct GLFWwindow;
constexpr uint32_t QUERY_COUNT = 128u;
static constexpr uint32_t VulkanApiVersion = VK_API_VERSION_1_3;

bool initVulkan(VulkanApp &app, const VulkanInitializationParameters &initParameters);
void deinitVulkan();
bool resizeSwapchain();
bool startRender();
void present(Image &imageToPresent);


void setObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, std::string_view name);
void setObjectTag(uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);
void beginDebugRegion(std::string_view pMarkerName, Vec4 color);
void insertDebugRegion(std::string_view markerName, Vec4 color);
void endDebugRegion();

void beginSingleTimeCommands();
void endSingleTimeCommands();








bool createGraphicsPipeline(const Shader& vertShader, const Shader& fragShader,
    const PodVector<VkFormat> &colorFormats, const DepthTest& depthTest, PipelineWithDescriptors &outPipeline);

bool createComputePipeline(const Shader& csShader, PipelineWithDescriptors& outPipeline);



bool createPipelineLayout(PipelineWithDescriptors &pipelineWithDescriptors, VkShaderStageFlags stage);


void destroyPipeline(PipelineWithDescriptors& pipelineWithDescriptors);
void destroyDescriptor(Descriptor &descriptor);


void bindPipelineWithDecriptors(VkPipelineBindPoint bindPoint, const PipelineWithDescriptors &pipelineWithDescriptor);

