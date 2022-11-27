#pragma once

#include <vulkan/vulkan_core.h>

#include <container/arraysliceview.h>
#include <core/mytypes.h>
#include <math/vector3.h>
#include <myvulkan/vulkanglobal.h>

static constexpr uint32_t QUERY_COUNT = 128u;
static constexpr uint32_t VulkanApiVersion = VK_API_VERSION_1_1;

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



bool initVulkan(VulkanApp &app);
void deinitVulkan();
VkRenderPass createRenderPass(const PodVector<RenderTarget>& colorTargets, const RenderTarget& depthFormat);
bool resizeSwapchain();
bool startRender();
void present(Image &imageToPresent);


void setObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name);
void setObjectTag(uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);
void beginDebugRegion(const char *pMarkerName, Vec4 color);
void insertDebugRegion(const char *markerName, Vec4 color);
void endDebugRegion();

void beginSingleTimeCommands();
void endSingleTimeCommands();

void beginRenderPass(const Pipeline& pipeline, const PodVector< VkClearValue >& clearValues);
void beginRendering(const PodVector<RenderImage>& renderColorImages, RenderImage depthImage);
void dispatchCompute(const Pipeline& pipeline, uint32_t bindSetIndex, uint32_t globalXSize, uint32_t globalYSize, uint32_t globalZSize,
    uint32_t localXSize, uint32_t localYSize, uint32_t localZSize);

bool createGraphicsPipeline(const Shader& vertShader, const Shader& fragShader,
    const PodVector< VkPipelineColorBlendAttachmentState > &blendChannels, const DepthTest &depthTest,
    Pipeline &outPipeline, const char *pipelineName,
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

bool createComputePipeline(const Shader& csShader, Pipeline& outPipeline, const char *pipelineName);



bool createPipelineLayout(Pipeline &pipelineWithDescriptors, VkShaderStageFlags stage);


void destroyPipeline(Pipeline& pipelineWithDescriptors);
void destroyDescriptor(Descriptor &descriptor);


void bindComputePipelineWithDecriptors(const Pipeline &pipelineWithDescriptor, uint32_t index);
void bindGraphicsPipelineWithDecriptors(const Pipeline &pipelineWithDescriptor, uint32_t index);

void writeStamp();