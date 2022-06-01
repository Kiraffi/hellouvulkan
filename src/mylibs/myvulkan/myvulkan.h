#pragma once

#include <string_view>
#include <vulkan/vulkan_core.h>

#include <container/arraysliceview.h>
#include <core/mytypes.h>
#include <math/vector3.h>
#include <myvulkan/vulkanglobal.h>

constexpr uint32_t QUERY_COUNT = 128u;
static constexpr uint32_t VulkanApiVersion = VK_API_VERSION_1_3;

class VulkanApp;
struct Shader;
struct GLFWwindow;

struct RenderTarget
{
    VkFormat format = VK_FORMAT_UNDEFINED;

    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
};

struct DepthTest
{
    RenderTarget depthTarget = {};
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    bool useDepthTest = false;
    bool writeDepth = false;
};

struct RenderImage
{
    const Image* const image = nullptr;
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkClearValue clearValue{};
};

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

void beginRenderPass(const Pipeline& pipeline, const PodVector< VkClearValue >& clearValues);
void beginRendering(const PodVector<RenderImage>& renderColorImages, RenderImage depthImage);

bool createGraphicsPipeline(const Shader& vertShader, const Shader& fragShader,
    const PodVector<RenderTarget> &colorTargets, const DepthTest& depthTest, Pipeline &outPipeline,
    std::string_view pipelineName, bool useDynamic = true);

bool createComputePipeline(const Shader& csShader, Pipeline& outPipeline, std::string_view pipelineName);



bool createPipelineLayout(Pipeline &pipelineWithDescriptors, VkShaderStageFlags stage);


void destroyPipeline(Pipeline& pipelineWithDescriptors);
void destroyDescriptor(Descriptor &descriptor);


void bindPipelineWithDecriptors(VkPipelineBindPoint bindPoint, const Pipeline &pipelineWithDescriptor);

