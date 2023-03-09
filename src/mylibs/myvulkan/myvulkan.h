#pragma once

#include <vulkan/vulkan_core.h>

#include <container/arraysliceview.h>
#include <core/mytypes.h>
#include <math/vector3.h>
#include <myvulkan/vulkanglobal.h>

static constexpr u32 QUERY_COUNT = 128u;
static constexpr u32 VulkanApiVersion = VK_API_VERSION_1_1;

struct Shader;

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

class MyVulkan
{
public:
    static bool init();
    static void deinit();

    static void setVulkanFrameResizedCBFunc(void (*fn)(i32 width, i32 height));

    static bool resizeSwapchain();
    static bool frameStart();
    static void present(Image &imageToPresent);

    static void setObjectName(u64 object,
        VkDebugReportObjectTypeEXT objectType,
        const char *name);
    static void setObjectTag(u64 object,
        VkDebugReportObjectTypeEXT objectType,
        u64 name,
        size_t tagSize,
        const void* tag);
    static void beginDebugRegion(const char *pMarkerName, Vec4 color);
    static void insertDebugRegion(const char *markerName, Vec4 color);
    static void endDebugRegion();

    static void beginSingleTimeCommands();
    static void endSingleTimeCommands();

    static VkRenderPass createRenderPass(
        const PodVector<RenderTarget>& colorTargets,
        const RenderTarget& depthFormat);

    static void beginRenderPass(
        const Pipeline& pipeline, const PodVector< VkClearValue >& clearValues);
    static void beginRendering(
        const PodVector<RenderImage>& renderColorImages, RenderImage depthImage);
    static void dispatchCompute(
        const Pipeline& pipeline, u32 bindSetIndex,
        u32 globalXSize, u32 globalYSize, u32 globalZSize,
        u32 localXSize, u32 localYSize, u32 localZSize);

    static bool createGraphicsPipeline(
        const Shader& vertShader, const Shader& fragShader,
        const PodVector< VkPipelineColorBlendAttachmentState > &blendChannels,
        const DepthTest &depthTest,
        Pipeline &outPipeline, const char *pipelineName,
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    static bool createComputePipeline(const Shader& csShader,
        Pipeline& outPipeline, const char *pipelineName);

    static bool createPipelineLayout(Pipeline &pipelineWithDescriptors, VkShaderStageFlags stage);

    static void destroyPipeline(Pipeline& pipelineWithDescriptors);
    static void destroyDescriptor(Descriptor &descriptor);

    static void bindComputePipelineWithDescriptors(
        const Pipeline &pipelineWithDescriptor, u32 index);
    static void bindGraphicsPipelineWithDescriptors(
        const Pipeline &pipelineWithDescriptor, u32 index);

    static void writeStamp();
};
