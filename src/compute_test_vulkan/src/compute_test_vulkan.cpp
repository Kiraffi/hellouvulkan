#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/camera.h>
#include <core/general.h>
#include <core/json.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/transform.h>
#include <core/vulkan_app.h>

#include <container/arraysliceview.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <model/gltf.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

#include <string.h>

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 600;



class VulkanComputeTest : public VulkanApp
{
public:
    VulkanComputeTest() { }
    virtual ~VulkanComputeTest() override;
    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual void resized() override;

    bool createPipelines();

    bool recreateDescriptor();

public:
    Camera camera;
    RenderModel renderModel;

    UniformBufferHandle uniformDataHandle;
    UniformBufferHandle animVertexDataHandle;
    UniformBufferHandle quadHandle;

    Buffer vertexBuffer;
    Buffer animationVertexBuffer;
    Buffer indexDataBuffer;
    Buffer quadIndexBuffer;

    PipelineWithDescriptors graphicsPipeline;

    PipelineWithDescriptors computePipeline;
    PipelineWithDescriptors graphicsFinalPipeline;

    Image renderColorImage;
    Image renderDepthImage;

    Image computeColorImage;
    VkSampler textureSampler = nullptr;
    VkSampler computeWriteSampler = nullptr;

    Image renderColorFinalImage;

    uint32_t indicesCount = 0;
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanComputeTest::~VulkanComputeTest()
{
    destroyPipeline(graphicsPipeline);
    destroyPipeline(computePipeline);
    destroyPipeline(graphicsFinalPipeline);

    destroyBuffer(vertexBuffer);
    destroyBuffer(animationVertexBuffer);
    destroyBuffer(indexDataBuffer);
    destroyBuffer(quadIndexBuffer);

    destroyImage(renderColorImage);
    destroyImage(renderDepthImage);

    destroyImage(computeColorImage);
    destroyImage(renderColorFinalImage);

    destroySampler(textureSampler);
    destroySampler(computeWriteSampler);
}



bool VulkanComputeTest::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;



    //bool readSuccess = readGLTF("assets/models/test_gltf.gltf", renderModel);
    //bool readSuccess = readGLTF("assets/models/arrows.gltf", renderModel);
    bool readSuccess = readGLTF("assets/models/animatedthing.gltf", renderModel);
    //bool readSuccess = readGLTF("assets/models/animatedthing_resaved.gltf", renderModel);

    printf("gltf read success: %i\n", readSuccess);
    if (!readSuccess)
        return false;

    uniformDataHandle = vulk.uniformBufferManager.reserveHandle();
    animVertexDataHandle = vulk.uniformBufferManager.reserveHandle();
    quadHandle = vulk.uniformBufferManager.reserveHandle();
    vertexBuffer = createBuffer(8u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex buffer");

    animationVertexBuffer = createBuffer(8u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Animation vertex buffer");

    indexDataBuffer = createBuffer(32 * 1024 * 1024,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");


    quadIndexBuffer = createBuffer(64 * 1024,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Quad index data buffer");

    // Random tag data
    //struct DemoTag { const char name[17] = "debug marker tag"; } demoTag;
    //setObjectTag(device, (uint64_t)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);


    {

        beginSingleTimeCommands();
        uint32_t quads[] = {1,0,2, 2,0,3};
        addToCopylist(ArraySliceViewBytes(quads, ARRAYSIZES(quads)), quadIndexBuffer.buffer, 0u);
        addToCopylist(sliceFromPodVectorBytes(renderModel.indices), indexDataBuffer.buffer, 0u);
        addToCopylist(sliceFromPodVectorBytes(renderModel.vertices), vertexBuffer.buffer, 0u);
        if(renderModel.animationVertices.size() > 0)
            addToCopylist(sliceFromPodVectorBytes(renderModel.animationVertices), animationVertexBuffer.buffer, 0u);
        flushBarriers(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        endSingleTimeCommands();

        indicesCount = renderModel.indices.size();
    }
    camera.position = Vec3(0.0f, 4.0f, 5.0f);

    return createPipelines();
}


bool VulkanComputeTest::createPipelines()
{
    {
        PipelineWithDescriptors &pipeline = graphicsPipeline;
        if (!createGraphicsPipeline(
            getShader(ShaderType::Basic3DAnimatedVert), getShader(ShaderType::Basic3DFrag),
            { vulk.defaultColorFormat },
            { .depthFormat = vulk.depthFormat, .useDepthTest = true, .writeDepth = true },
            pipeline))
        {
            printf("failed to create pipeline\n");
        }


        pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
            {
                DescriptorInfo(vulk.renderFrameBufferHandle),
                DescriptorInfo(uniformDataHandle),
                DescriptorInfo(animVertexDataHandle),

                DescriptorInfo(vertexBuffer.buffer, 0u, vertexBuffer.size),
                DescriptorInfo(animationVertexBuffer.buffer, 0u, animationVertexBuffer.size),
            });

        pipeline.descriptor = createDescriptor(pipeline.descriptorSetLayouts, pipeline.descriptorSetLayout);
        if (!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }
    {
        PipelineWithDescriptors &pipeline = computePipeline;
        if(!createComputePipeline(getShader(ShaderType::ComputeTestComp), pipeline))
        {
            printf("Failed to create compute pipeline!\n");
            return false;
        }

        VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        pipeline.descriptor = createDescriptor(pipeline.descriptorSetLayouts, pipeline.descriptorSetLayout);

        textureSampler = createSampler(samplerInfo);
        if(!textureSampler)
        {
            printf("Failed to create sampler for font rendering");
            return false;
        }

        {
            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.minFilter = VK_FILTER_NEAREST;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_FALSE; //VK_TRUE;
            samplerInfo.maxAnisotropy = 1;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerInfo.minLod = 0;
            samplerInfo.maxLod = 1.0f;
            samplerInfo.mipLodBias = 0;

            VK_CHECK(vkCreateSampler(vulk.device, &samplerInfo, nullptr, &computeWriteSampler));
        }

    }

    // Create copy-pipelines
    {
        PipelineWithDescriptors &pipeline = graphicsFinalPipeline;
        if (!createGraphicsPipeline(
            getShader(ShaderType::TexturedQuadVert), getShader(ShaderType::TexturedQuadFrag),
            { vulk.defaultColorFormat }, {}, pipeline))
        {
            printf("Failed to create graphics pipeline\n");
            return false;
        }

    }

    return recreateDescriptor();
}

bool VulkanComputeTest::recreateDescriptor()
{
    if(!textureSampler)
        return false;

    {
        PipelineWithDescriptors &pipeline = computePipeline;
        destroyDescriptor(pipeline.descriptor);
        pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
            {
    //            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                DescriptorInfo(vulk.renderFrameBufferHandle),
                DescriptorInfo(renderColorImage.imageView, VK_IMAGE_LAYOUT_GENERAL, textureSampler),
                DescriptorInfo(computeColorImage.imageView, VK_IMAGE_LAYOUT_GENERAL, computeWriteSampler),
            });
        pipeline.descriptor = createDescriptor(pipeline.descriptorSetLayouts, pipeline.descriptorSetLayout);
        if (!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }
    {
        PipelineWithDescriptors &pipeline = graphicsFinalPipeline;
        pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
        {
            DescriptorInfo(vulk.renderFrameBufferHandle),
            DescriptorInfo(quadHandle),
            DescriptorInfo(computeColorImage.imageView, VK_IMAGE_LAYOUT_GENERAL, textureSampler),
        });

        pipeline.descriptor = createDescriptor(pipeline.descriptorSetLayouts, pipeline.descriptorSetLayout);
        if(!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }

    }
    return true;
}

void VulkanComputeTest::resized()
{
    destroyImage(renderColorImage);
    destroyImage(renderDepthImage);
    destroyImage(computeColorImage);
    destroyImage(renderColorFinalImage);

    // create color and depth images
    renderColorImage = createImage(
        vulk.swapchain.width, vulk.swapchain.height,
        vulk.defaultColorFormat,

        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main color target image");

    renderDepthImage = createImage(
        vulk.swapchain.width, vulk.swapchain.height, vulk.depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main depth target image");

    computeColorImage = createImage(
        vulk.swapchain.width, vulk.swapchain.height,
        vulk.defaultColorFormat,

        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Compute target image");

    renderColorFinalImage = createImage(
        vulk.swapchain.width, vulk.swapchain.height,
        vulk.defaultColorFormat,

        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Render color final image");

    recreateDescriptor();
}

void VulkanComputeTest::logicUpdate()
{
    VulkanApp::logicUpdate();

    MouseState mouseState = getMouseState();

    checkCameraKeypresses(dt, camera);

    camera.renderCameraInfo(fontSystem, Vec2(10.0f, 10.0f), Vec2(8.0f, 12.0f));
}

void VulkanComputeTest::renderUpdate()
{
    VulkanApp::renderUpdate();


    struct FrameBuffer
    {
        Matrix camMat;
        Matrix viewProj;
        Matrix mvp;
        Matrix padding;
    };
    FrameBuffer frameBufferData;



    frameBufferData.camMat = camera.getCameraMatrix();

    const SwapChain &swapchain = vulk.swapchain;
    camera.aspectRatioWByH = float(swapchain.width) / float(swapchain.height);
    camera.fovY = 90.0f;
    camera.zFar = 200.0f;
    camera.zNear = 0.001f;

    frameBufferData.viewProj = camera.perspectiveProjection();
    frameBufferData.mvp = frameBufferData.viewProj * frameBufferData.camMat;

    Transform trans;
    trans.pos = Vec3(3.0f, 0.0f, -5.0f);
    static float rotationAmount = Pi * 0.25f;

    trans.rot = getQuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), rotationAmount);
    trans.scale = Vec3(1.0f, 1.0f, 1.0f);

    rotationAmount += 1.5f * dt;

    frameBufferData.padding = getModelMatrix(trans);

    addToCopylist(frameBufferData, uniformDataHandle);


    PodVector<Matrix> animationMatrices;
    if(!evaluateAnimation(renderModel, 0, this->getTime(), animationMatrices))
    {
        animationMatrices.resize(256, Matrix());
    }
    addToCopylist(sliceFromPodVectorBytes(animationMatrices), animVertexDataHandle);

    FontRenderSystem::GPUVertexData vdata;
    vdata.pos = Vec2(0.0f, 0.0f); // Vec2(swapchain.width / 2.0f, swapchain.height / 2.0f);
    vdata.pixelSizeX = swapchain.width;
    vdata.pixelSizeY = swapchain.height;
    vdata.color = getColor(1.0f, 1.0f, 1.0f, 1.0f);
    vdata.uvStart = Vec2(0.0f, 0.0f);
    vdata.uvSize = Vec2(1.0f, 1.0f);

    addToCopylist(vdata, quadHandle);
}

void VulkanComputeTest::renderDraw()
{
    const SwapChain& swapchain = vulk.swapchain;

    addImageBarrier(imageBarrier(renderColorImage,
        0, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

    addImageBarrier(imageBarrier(renderDepthImage,
        0, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT));

    flushBarriers();

    // Drawingg
    {
        static constexpr VkClearValue colorClear = { .color{0.0f, 0.5f, 1.0f, 1.0f} };
        static constexpr VkClearValue depthClear = { .depthStencil = { 1.0f, 0 } };
        VkRect2D renderArea = { .extent = {.width = swapchain.width, .height = swapchain.height } };
        VkViewport viewPort = { 0.0f, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.0f, 1.0f };
        VkRect2D scissors = { { 0, 0 }, { uint32_t(swapchain.width), uint32_t(swapchain.height) } };


        const VkRenderingAttachmentInfo colorAttachmentInfo[] {
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .imageView = renderColorImage.imageView,
                .imageLayout = renderColorImage.layout,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = colorClear
            },

        };
        const VkRenderingAttachmentInfo depthAttachment{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = renderDepthImage.imageView,
            .imageLayout = renderDepthImage.layout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = depthClear
        };

        const VkRenderingInfo renderInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea = renderArea,
            .layerCount = 1,
            .colorAttachmentCount = ARRAYSIZES(colorAttachmentInfo),
            .pColorAttachments = colorAttachmentInfo,
            .pDepthAttachment = &depthAttachment,
        };

        vkCmdBeginRendering(vulk.commandBuffer, &renderInfo);

        insertDebugRegion("Render", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        vkCmdSetViewport(vulk.commandBuffer, 0, 1, &viewPort);
        vkCmdSetScissor(vulk.commandBuffer, 0, 1, &scissors);
        // draw calls here
        // Render
        {
            bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            vkCmdBindIndexBuffer(vulk.commandBuffer, indexDataBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulk.commandBuffer, indicesCount, 1, 0, 0, 0);

        }
        vkCmdEndRendering(vulk.commandBuffer);

        fontSystem.render(renderColorImage);
    }

    writeStamp(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    {
        addImageBarrier(imageBarrier(renderColorImage,
            //VK_ACCESS_MEMORY_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));

        addImageBarrier(imageBarrier(computeColorImage,
            0, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL));
            //VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL));

        flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);


        bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdDispatch(vulk.commandBuffer, (swapchain.width + 7) / 8, (swapchain.height + 7) / 8, 1);

    }
    writeStamp(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    {
        addImageBarrier(imageBarrier(renderColorFinalImage,
            0, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

        addImageBarrier(imageBarrier(computeColorImage,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));

        flushBarriers(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

        static constexpr VkClearValue colorClear = { .color{0.0f, 0.5f, 1.0f, 1.0f} };
        static constexpr VkClearValue depthClear = { .depthStencil = { 1.0f, 0 } };
        VkRect2D renderArea = { .extent = {.width = swapchain.width, .height = swapchain.height } };
        VkViewport viewPort = { 0.0f, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.0f, 1.0f };
        VkRect2D scissors = { { 0, 0 }, { uint32_t(swapchain.width), uint32_t(swapchain.height) } };


        const VkRenderingAttachmentInfo colorAttachmentInfo[] {
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .imageView = renderColorFinalImage.imageView,
                .imageLayout = renderColorFinalImage.layout,
                .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = colorClear
            },
        };

        const VkRenderingInfo renderInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea = renderArea,
            .layerCount = 1,
            .colorAttachmentCount = ARRAYSIZES(colorAttachmentInfo),
            .pColorAttachments = colorAttachmentInfo,
            .pDepthAttachment = nullptr,
        };

        vkCmdBeginRendering(vulk.commandBuffer, &renderInfo);

        insertDebugRegion("RenderCopy", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        vkCmdSetViewport(vulk.commandBuffer, 0, 1, &viewPort);
        vkCmdSetScissor(vulk.commandBuffer, 0, 1, &scissors);
        // draw calls here
        // Render
        {
            bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsFinalPipeline);
            vkCmdBindIndexBuffer(vulk.commandBuffer, quadIndexBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulk.commandBuffer, 6, 1, 0, 0, 0);
        }
        vkCmdEndRendering(vulk.commandBuffer);

    }

    writeStamp(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    present(renderColorFinalImage);
}





int main(int argCount, char **argv)
{
    VulkanComputeTest app;
    if (app.init("Vulkan, render font", SCREEN_WIDTH, SCREEN_HEIGHT,
        {
            .showInfoMessages = false,
            .useHDR = false,
            .useIntegratedGpu = false,
            .useValidationLayers = true,
            .useVulkanDebugMarkersRenderDoc = false,
            .vsync = VSyncType::IMMEDIATE_NO_VSYNC
        }))
    {
        app.run();
    }

    return 0;
}