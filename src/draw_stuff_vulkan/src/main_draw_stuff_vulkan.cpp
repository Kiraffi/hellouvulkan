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
#include <myvulkan/vulkanresources.h>

#include <string.h>

static constexpr int SCREEN_WIDTH = 640;
static constexpr int SCREEN_HEIGHT = 540;



class VulkanDrawStuff : public VulkanApp
{
public:
    VulkanDrawStuff() { }
    virtual ~VulkanDrawStuff() override;
    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual void resized() override;

    bool createPipelines();

public:
    Camera camera;

    UniformBufferHandle uniformDataHandle;
    UniformBufferHandle animVertexDataHandle;
    VkShaderModule vertShaderModule = { };
    VkShaderModule fragShaderModule = { };

    Buffer vertexBuffer;
    Buffer animationVertexBuffer;
    Buffer indexDataBuffer;

    PipelineWithDescriptors graphicsPipeline;

    Image renderColorImage;
    Image renderDepthImage;

    uint32_t indicesCount = 0;
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanDrawStuff::~VulkanDrawStuff()
{
    destroyDescriptor(graphicsPipeline.descriptor);
    destroyPipeline(graphicsPipeline);

    destroyBuffer(vertexBuffer);
    destroyBuffer(animationVertexBuffer);
    destroyBuffer(indexDataBuffer);

    destroyImage(renderColorImage);
    destroyImage(renderDepthImage);

    vkDestroyShaderModule(vulk.device, vertShaderModule, nullptr);
    vkDestroyShaderModule(vulk.device, fragShaderModule, nullptr);

}



bool VulkanDrawStuff::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;


    RenderModel renderModel;

    //bool readSuccess = readGLTF("assets/models/test_gltf.gltf", renderModel);
    //bool readSuccess = readGLTF("assets/models/arrows.gltf", renderModel);
    //bool readSuccess = readGLTF("assets/models/animatedthing.gltf", renderModel);
    bool readSuccess = readGLTF("assets/models/animatedthing_resaved.gltf", renderModel);

    printf("gltf read success: %i\n", readSuccess);
    if (!readSuccess)
        return false;

    //vertShaderModule = loadShader("assets/shader/vulkan_new/basic3d.vert.spv");
    //ASSERT(vertShaderModule);
    vertShaderModule = loadShader("assets/shader/vulkan_new/basic3d_animated.vert.spv");
    ASSERT(vertShaderModule);

    fragShaderModule = loadShader("assets/shader/vulkan_new/basic3d.frag.spv");
    ASSERT(fragShaderModule);

    uniformDataHandle = vulk.uniformBufferManager.reserveHandle();
    animVertexDataHandle = vulk.uniformBufferManager.reserveHandle();
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

    // Random tag data
    //struct DemoTag { const char name[17] = "debug marker tag"; } demoTag;
    //setObjectTag(device, (uint64_t)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);


    {

        beginSingleTimeCommands();

        addToCopylist(sliceFromPodVectorBytes(renderModel.indices), indexDataBuffer.buffer, 0u);
        addToCopylist(sliceFromPodVectorBytes(renderModel.vertices), vertexBuffer.buffer, 0u);
        addToCopylist(sliceFromPodVectorBytes(renderModel.animationVertices), animationVertexBuffer.buffer, 0u);
        flushBarriers(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        endSingleTimeCommands();

        indicesCount = renderModel.indices.size();
    }
    camera.position = Vec3(0.0f, 4.0f, 5.0f);

    return createPipelines();
}


bool VulkanDrawStuff::createPipelines()
{
    PipelineWithDescriptors &pipeline = graphicsPipeline;

    pipeline.descriptorSetLayouts = PodVector<DescriptorSetLayout>(
    {
        DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u },
        DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u },
        DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u },

        DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3u },
        DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4u },
    });

    if (!createPipelineLayout(pipeline, VK_SHADER_STAGE_ALL_GRAPHICS))
    {
        printf("Failed to create pipelinelayout!\n");
        return false;
    }

    pipeline.pipeline = createGraphicsPipeline(
        vertShaderModule, fragShaderModule,
        pipeline.pipelineLayout,
        { vulk.defaultColorFormat },
        {.depthFormat = vulk.depthFormat, .useDepthTest = true, .writeDepth = true}
    );


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
    return true;
}

void VulkanDrawStuff::resized()
{
    destroyImage(renderColorImage);
    destroyImage(renderDepthImage);

    // create color and depth images
    renderColorImage = createImage(
        vulk.swapchain.width, vulk.swapchain.height,
        vulk.defaultColorFormat,

        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main color target image");

    renderDepthImage = createImage(
        vulk.swapchain.width, vulk.swapchain.height, vulk.depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main depth target image");

}

void VulkanDrawStuff::logicUpdate()
{
    VulkanApp::logicUpdate();

    MouseState mouseState = getMouseState();

    checkCameraKeypresses(dt, camera);

    Transform trans;
    trans.pos = Vec3(3.0f, 3.0f, 13.0f);
    static float rotationAmount = Pi * 0.25f;

    trans.rot = getQuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), rotationAmount);
    Vec3 tmp = rotateVector(Vector3(0.0f, 0.0f, 1.0f), trans.rot);
    trans.scale = Vec3(1.0f, 1.0f, 1.0f);

    Transform trans2;
    trans2.pos = Vec3(5.0f, 0.0f, 0.0f);
    rotationAmount += 1.5f * dt;

    //b.padding = getModelMatrix(trans); // *getModelMatrix(trans);

    camera.renderCameraInfo(fontSystem, Vec2(10.0f, 10.0f), Vec2(8.0f, 12.0f));
}

void VulkanDrawStuff::renderUpdate()
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

    frameBufferData.viewProj = camera.perspectiveProjectionRH();
    frameBufferData.mvp = frameBufferData.camMat * frameBufferData.viewProj;

    addToCopylist(frameBufferData, uniformDataHandle);


    PodVector<Matrix> animationMatrices;
    animationMatrices.resize(256, Matrix());

    addToCopylist(sliceFromPodVectorBytes(animationMatrices), animVertexDataHandle);
}

void VulkanDrawStuff::renderDraw()
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

    present(renderColorImage);
}





int main(int argCount, char **argv)
{
    VulkanDrawStuff app;
    if (app.init("Vulkan, render font", SCREEN_WIDTH, SCREEN_HEIGHT,
        {
            .showInfoMessages = false,
            .useHDR = false,
            .useIntegratedGpu = true,
            .useValidationLayers = true,
            .useVulkanDebugMarkersRenderDoc = false,
            .vsync = VSyncType::IMMEDIATE_NO_VSYNC
        }))
    {
        app.run();
    }

    return 0;
}