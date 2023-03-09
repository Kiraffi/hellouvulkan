#include <pch.h>

#include <app/glfw_keys.h>
#include <app/inputapp.h>
#include <app/vulkan_app.h>

#include <components/transform.h>

#include <container/podvector.h>
#include <container/vector.h>

#include <core/camera.h>
#include <core/general.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <app/vulkan_app.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <model/gltf.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkaninitparameters.h>
#include <myvulkan/vulkanresources.h>

#include <render/fontrendersystem.h>
#include <render/meshrendersystem.h>

#include <resources/globalresources.h>

#include <scene/scene.h>



static constexpr i32 c_ScreenWidth = 800;
static constexpr i32 c_ScreenHeight = 600;

static void sRunApp();
static void sHandleInput();
static void sDraw();
static void sRenderUpdate();

static bool sInit(const char *windowStr, i32 screenWidth, i32 screenHeight);
static void sDeinit();
static bool sResize();
static void sResized(int width, int height);

struct ComputeTestData
{
    Scene m_scene;
    MeshRenderSystem m_meshRenderSystem;

    UniformBufferHandle m_quadHandle;

    Buffer m_quadIndexBuffer;

    Pipeline m_computePipeline;
    Pipeline m_graphicsFinalPipeline;

    MeshRenderTargets m_meshRenderTargets;

    Image m_computeColorImage;

    Image m_renderColorFinalImage;

    Camera m_camera;
};

static ComputeTestData *s_computeData = nullptr;

i32 main(i32 argCount, char **argv)
{
    initMemory();
    initGlobalResources();

    auto &vulkanInitParams = VulkanInitializationParameters::getRef();

    if(sInit("Vulkan, compute test", c_ScreenWidth, c_ScreenHeight))
    {
        sRunApp();
    }

    sDeinit();
    deinitGlobalResources();

    deinitMemory();
    return 0;
}

static void sRunApp()
{
    while(VulkanApp::updateApp())
    {
        sHandleInput();

        if(MyVulkan::frameStart())
        {
            //updateStats(*this);
            vulk->currentStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            sRenderUpdate();
            sDraw();
            //renderDraw();
            //printStats(*this);
        }

        defragMemory();
        VulkanApp::frameEnd();
    }
    VK_CHECK(vkDeviceWaitIdle(vulk->device));
}



////////////////////////
//
// DEINIT
//
////////////////////////

static void sDeinit()
{
    FontRenderSystem::deinit();
    MeshRenderSystem::deinit();
    if(s_computeData)
    {
        MyVulkan::destroyPipeline(s_computeData->m_computePipeline);
        MyVulkan::destroyPipeline(s_computeData->m_graphicsFinalPipeline);

        VulkanResources::destroyBuffer(s_computeData->m_quadIndexBuffer);

        VulkanResources::destroyImage(s_computeData->m_computeColorImage);
        VulkanResources::destroyImage(s_computeData->m_renderColorFinalImage);
        delete s_computeData;
        s_computeData = nullptr;
    }

    MyVulkan::deinit();
    VulkanApp::deinitApp();
}


static bool sInit(const char *windowStr, i32 screenWidth, i32 screenHeight)
{
    s_computeData = new ComputeTestData();

    if(!VulkanApp::initApp("Vulkan, draw font", c_ScreenWidth, c_ScreenHeight)
        || !InputApp::init()
        || !MyVulkan::init()
        || !FontRenderSystem::init("assets/font/new_font.dat")
        || !MeshRenderSystem::init())
    {
        return false;
    }
    
    s_computeData->m_quadHandle = vulk->uniformBufferManager.reserveHandle();

    s_computeData->m_quadIndexBuffer = VulkanResources::createBuffer(64 * 1024,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Quad index data buffer");

    // Random tag data
    //struct DemoTag { const char name[17] = "debug marker tag"; } demoTag;
    //setObjectTag(device, (u64)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);


    {

        MyVulkan::beginSingleTimeCommands();
        u32 quads[] = {1,0,2, 2,0,3};
        VulkanResources::addToCopylist(ArraySliceViewBytes(quads, ARRAYSIZES(quads)), s_computeData->m_quadIndexBuffer.buffer, 0u);
        VulkanResources::flushBarriers(VK_PIPELINE_STAGE_TRANSFER_BIT);

        MyVulkan::endSingleTimeCommands();
    }
    s_computeData->m_camera.m_position = Vec3(0.0f, 4.0f, 5.0f);
    
    if(!s_computeData->m_scene.init())
        return false;

    s_computeData->m_scene.addGameEntity({ .transform = {.pos = {3.0f, 0.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING });



    {
        Pipeline &pipeline = s_computeData->m_computePipeline;
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

        if(!MyVulkan::createComputePipeline(VulkanShader::getShader(ShaderType::ComputeTestComp), pipeline, "Compute copy pipeline test"))
        {
            printf("Failed to create compute pipeline!\n");
            return false;
        }
    }

    // Create copy-pipelines
    {
        Pipeline &pipeline = s_computeData->m_graphicsFinalPipeline;
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

        pipeline.renderPass = MyVulkan::createRenderPass(
            { RenderTarget{ .format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD } },
            {});
        ASSERT(pipeline.renderPass);
        if(!pipeline.renderPass)
            return false;

        VkPipelineColorBlendAttachmentState rgbaAtt{ .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        if(!MyVulkan::createGraphicsPipeline(
            VulkanShader::getShader(ShaderType::TexturedQuadVert), VulkanShader::getShader(ShaderType::TexturedQuadFrag),
            { rgbaAtt }, {}, pipeline, "Graphics copy pipeline test"))
        {
            printf("Failed to create graphics pipeline\n");
            return false;
        }
    }
    MyVulkan::setVulkanFrameResizedCBFunc(sResized);

    return sResize();
}

static void sResized(int w, int h)
{
    sResize();
}

static bool sResize()
{
    u32 width = vulk->swapchain.width;
    u32 height = vulk->swapchain.height;

    if(!s_computeData->m_meshRenderTargets.resizeMeshTargets(width, height))
        return false;

    if(!s_computeData->m_meshRenderTargets.resizeShadowTarget(2048, 2048))
        return false;

    if(!VulkanResources::createRenderTargetImage(
        width, height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Compute target image", s_computeData->m_computeColorImage))
    {
        printf("Failed to create: %s\n", s_computeData->m_computeColorImage.imageName);
        return false;
    }

    if(!VulkanResources::createRenderTargetImage(width, height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Render color final image", s_computeData->m_renderColorFinalImage))
    {
        printf("Failed to create: %s\n", s_computeData->m_renderColorFinalImage.imageName);
        return false;
    }

    MeshRenderSystem::setRenderTargets(s_computeData->m_meshRenderTargets);

    FontRenderSystem::setRenderTarget(s_computeData->m_meshRenderTargets.albedoImage);
    ASSERT(VulkanResources::createFramebuffer(s_computeData->m_graphicsFinalPipeline, { s_computeData->m_renderColorFinalImage }));

    if(!s_computeData->m_computePipeline.pipeline || !s_computeData->m_graphicsFinalPipeline.pipeline)
        return false;

    {
        Pipeline &pipeline = s_computeData->m_computePipeline;
        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);

        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),
                //DescriptorInfo(renderColorImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureSampler),
                DescriptorInfo(s_computeData->m_meshRenderTargets.albedoImage.imageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE),
                DescriptorInfo(s_computeData->m_computeColorImage.imageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE),
            };
        }

        if(!VulkanShader::updateBindDescriptorSet(pipeline))
            return false;
    }
    {
        Pipeline &pipeline = s_computeData->m_graphicsFinalPipeline;
        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);

        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),
                DescriptorInfo(s_computeData->m_quadHandle),
                DescriptorInfo(s_computeData->m_computeColorImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),
            };
        }

        if(!VulkanShader::updateBindDescriptorSet(pipeline))
            return false;
    }

    if(s_computeData->m_quadHandle.isValid())
    {
        FontRenderSystem::GPUVertexData vdata;
        vdata.pos = Vec2(0.0f, 0.0f); // Vec2(swapchain.width / 2.0f, swapchain.height / 2.0f);
        vdata.pixelSizeX = vulk->swapchain.width;
        vdata.pixelSizeY = vulk->swapchain.height;
        vdata.color = getColor(1.0f, 1.0f, 1.0f, 1.0f);
        vdata.uvStart = Vec2(0.0f, 0.0f);
        vdata.uvSize = Vec2(1.0f, 1.0f);

        VulkanResources::addToCopylist(vdata, s_computeData->m_quadHandle);
    }

    return true;
}

static void sHandleInput()
{
    MeshRenderSystem::clear();

    MouseState mouseState = InputApp::getMouseState();
    float dt = VulkanApp::getWindowApp().frameDt;

    s_computeData->m_camera.checkCameraKeypresses();

    if(InputApp::isPressed(GLFW_KEY_KP_ADD))
    {
        for(auto &entity : s_computeData->m_scene.getEntities())
        {
            ++entity.animationIndex;
        }
    }
    if(InputApp::isPressed(GLFW_KEY_KP_SUBTRACT))
    {
        for(auto &entity : s_computeData->m_scene.getEntities())
        {
            if(entity.animationIndex > 0)
                --entity.animationIndex;
        }
    }

    s_computeData->m_camera.renderCameraInfo(Vec2(10.0f, 10.0f), Vec2(8.0f, 12.0f));
}

static void sRenderUpdate()
{
    const auto &app = VulkanApp::getWindowApp();
    float dt = app.frameDt;

    vulk->queryPoolIndexCounts[vulk->frameIndex] = 0u;

    //beginSingleTimeCommands();
    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(vulk->commandBuffer, &beginInfo));

    vkCmdResetQueryPool(vulk->commandBuffer, vulk->queryPools[vulk->frameIndex], 0, QUERY_COUNT);

    MyVulkan::writeStamp();
    VulkanResources::update();

    struct FrameBuffer
    {
        Vector2 areaSize;
        Vector2 tmp1;
        Vector4 tmp2[3];

        Matrix camMat;
        Matrix viewProj;
        Matrix mvp;
        Matrix sunMatrix;


        Vector4 camPos;
        Vector4 tmp3[3];

        Matrix inverseMvp;
    };
    FrameBuffer frameBufferData;


    if(app.windowWidth > 0 && app.windowHeight > 0)
        s_computeData->m_camera.updateCameraState(app.windowWidth, app.windowHeight);

    frameBufferData.mvp = s_computeData->m_camera.m_worldToViewMat;
    frameBufferData.inverseMvp = s_computeData->m_camera.m_viewToWorldMat;

    frameBufferData.camPos = Vector4(s_computeData->m_camera.m_position, 0.0f);

    frameBufferData.areaSize = Vec2(app.windowWidth, app.windowHeight);
    VulkanResources::addToCopylist(frameBufferData, vulk->renderFrameBufferHandle[vulk->frameIndex]);

    FontRenderSystem::update();

    s_computeData->m_scene.update(dt);
    MeshRenderSystem::prepareToRender();

}

static void sDraw()
{
    const SwapChain& swapchain = vulk->swapchain;

    // Drawingg
    {
        s_computeData->m_meshRenderTargets.prepareTargetsForMeshRendering();
        MeshRenderSystem::render(s_computeData->m_meshRenderTargets);
        FontRenderSystem::render();
    }

    {
        VulkanResources::prepareToComputeImageRead(s_computeData->m_meshRenderTargets.albedoImage);
        VulkanResources::prepareToComputeImageWrite(s_computeData->m_computeColorImage);
        MyVulkan::dispatchCompute(
            s_computeData->m_computePipeline,
            vulk->frameIndex,
            swapchain.width, swapchain.height, 1,
            8, 8, 1);
        MyVulkan::writeStamp();
    }

    {
        VulkanResources::prepareToGraphicsSampleRead(s_computeData->m_computeColorImage);
        VulkanResources::prepareToGraphicsSampleWrite(s_computeData->m_renderColorFinalImage);

        MyVulkan::beginRenderPass(s_computeData->m_graphicsFinalPipeline, {});
        //beginRendering({ RenderImage{ .image = &renderColorFinalImage, .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE } }, {});
        MyVulkan::insertDebugRegion("RenderCopy", Vec4(1.0f, 0.0f, 0.0f, 1.0f));

        // draw calls here
        // Render
        {
            MyVulkan::bindGraphicsPipelineWithDescriptors(s_computeData->m_graphicsFinalPipeline, vulk->frameIndex);
            vkCmdBindIndexBuffer(vulk->commandBuffer, s_computeData->m_quadIndexBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulk->commandBuffer, 6, 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(vulk->commandBuffer);
        //vkCmdEndRendering(vulk->commandBuffer);
        MyVulkan::writeStamp();
    }


    //present(meshRenderTargets.albedoImage);
    //present(computeColorImage);
    MyVulkan::present(s_computeData->m_renderColorFinalImage);
}
