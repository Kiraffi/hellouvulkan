#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <components/transform.h>

#include <core/camera.h>
#include <core/general.h>
#include <core/timer.h>
#include <core/mytypes.h>
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

#include <render/meshrendersystem.h>
#include <scene/scene.h>


static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 600;



class VulkanComputeTest : public VulkanApp
{
public:
    VulkanComputeTest() : scene(meshRenderSystem) { }
    virtual ~VulkanComputeTest() override;
    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

    bool createPipelines();

    bool recreateDescriptor();

public:
    Scene scene;
    MeshRenderSystem meshRenderSystem;

    UniformBufferHandle quadHandle;

    Buffer quadIndexBuffer;

    Pipeline computePipeline;
    Pipeline graphicsFinalPipeline;

    MeshRenderTargets meshRenderTargets;

    Image computeColorImage;

    Image renderColorFinalImage;
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanComputeTest::~VulkanComputeTest()
{
    destroyPipeline(computePipeline);
    destroyPipeline(graphicsFinalPipeline);

    destroyBuffer(quadIndexBuffer);

    destroyImage(computeColorImage);
    destroyImage(renderColorFinalImage);
}



bool VulkanComputeTest::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;


    if (!meshRenderSystem.init())
        return false;

    quadHandle = vulk->uniformBufferManager.reserveHandle();

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
        flushBarriers(VK_PIPELINE_STAGE_TRANSFER_BIT);

        endSingleTimeCommands();
    }
    camera.position = Vec3(0.0f, 4.0f, 5.0f);

    if (!scene.init())
        return false;
    scene.addGameEntity({ .transform = {.pos = {3.0f, 0.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING });

    return createPipelines();
}


bool VulkanComputeTest::createPipelines()
{
    if(!resized())
        return false;
    {
        Pipeline &pipeline = computePipeline;
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

        if(!createComputePipeline(getShader(ShaderType::ComputeTestComp), pipeline, "Compute copy pipeline test"))
        {
            printf("Failed to create compute pipeline!\n");
            return false;
        }
    }

    // Create copy-pipelines
    {
        Pipeline &pipeline = graphicsFinalPipeline;
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

        if (!createGraphicsPipeline(
            getShader(ShaderType::TexturedQuadVert), getShader(ShaderType::TexturedQuadFrag),
            { RenderTarget{.format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE } },
            {}, pipeline, "Graphics copy pipeline test", true))
        {
            printf("Failed to create graphics pipeline\n");
            return false;
        }
    }

    return recreateDescriptor(); // resized() && recreateDescriptor();
}

bool VulkanComputeTest::recreateDescriptor()
{
    if(!computePipeline.pipeline || !graphicsFinalPipeline.pipeline)
        return false;

    {
        Pipeline &pipeline = computePipeline;
        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);

        for(uint32_t i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),
                //DescriptorInfo(renderColorImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureSampler),
                DescriptorInfo(meshRenderTargets.albedoImage.imageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE),
                DescriptorInfo(computeColorImage.imageView, VK_IMAGE_LAYOUT_GENERAL, VK_NULL_HANDLE),
            };
        }

        if(!updateBindDescriptorSet(pipeline))
            return false;
    }
    {
        Pipeline &pipeline = graphicsFinalPipeline;
        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);

        for(uint32_t i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),
                DescriptorInfo(quadHandle),
                DescriptorInfo(computeColorImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulk->globalTextureSampler),
            };
        }

        if(!updateBindDescriptorSet(pipeline))
            return false;
    }
    return true;
}

bool VulkanComputeTest::resized()
{
    uint32_t width = vulk->swapchain.width;
    uint32_t height = vulk->swapchain.height;

    if (!meshRenderTargets.resizeMeshTargets(width, height))
        return false;



    if (!createRenderTargetImage(
        width, height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Compute target image", computeColorImage))
    {
        printf("Failed to create: %s\n", computeColorImage.imageName);
        return false;
    }

    if (!createRenderTargetImage(width, height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Render color final image", renderColorFinalImage))
    {
        printf("Failed to create: %s\n", renderColorFinalImage.imageName);
        return false;
    }

    fontSystem.setRenderTarget(meshRenderTargets.albedoImage);
    //ASSERT(createFramebuffer(graphicsFinalPipeline, { renderColorFinalImage }));
    recreateDescriptor();

    if (quadHandle.isValid())
    {
        FontRenderSystem::GPUVertexData vdata;
        vdata.pos = Vec2(0.0f, 0.0f); // Vec2(swapchain.width / 2.0f, swapchain.height / 2.0f);
        vdata.pixelSizeX = vulk->swapchain.width;
        vdata.pixelSizeY = vulk->swapchain.height;
        vdata.color = getColor(1.0f, 1.0f, 1.0f, 1.0f);
        vdata.uvStart = Vec2(0.0f, 0.0f);
        vdata.uvSize = Vec2(1.0f, 1.0f);

        addToCopylist(vdata, quadHandle);
    }
    return true;
}

void VulkanComputeTest::logicUpdate()
{
    VulkanApp::logicUpdate();

    MouseState mouseState = getMouseState();

    checkCameraKeypresses(dt, camera);

    camera.renderCameraInfo(fontSystem, Vec2(10.0f, 10.0f), Vec2(8.0f, 12.0f));

    meshRenderSystem.clear();
}

void VulkanComputeTest::renderUpdate()
{
    VulkanApp::renderUpdate();
    scene.update(dt);
    meshRenderSystem.prepareToRender();
}

void VulkanComputeTest::renderDraw()
{
    const SwapChain& swapchain = vulk->swapchain;

    // Drawingg
    {
        meshRenderTargets.prepareTargetsForMeshRendering();
        meshRenderSystem.render(meshRenderTargets);

        prepareToGraphicsSampleWrite(meshRenderTargets.albedoImage);
        flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
        fontSystem.render();
    }


    {
        prepareToComputeImageRead(meshRenderTargets.albedoImage);
        prepareToComputeImageWrite(computeColorImage);
        dispatchCompute(computePipeline, vulk->frameIndex, swapchain.width, swapchain.height, 1, 8, 8, 1);
        writeStamp();
    }

    {
        prepareToGraphicsSampleRead(computeColorImage);
        prepareToGraphicsSampleWrite(renderColorFinalImage);
        
        //beginRenderPass(graphicsFinalPipeline, {});
        beginRendering({ RenderImage{ .image = &renderColorFinalImage, .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE } }, {});
        insertDebugRegion("RenderCopy", Vec4(1.0f, 0.0f, 0.0f, 1.0f));

        // draw calls here
        // Render
        {
            bindGraphicsPipelineWithDecriptors(graphicsFinalPipeline, vulk->frameIndex);
            vkCmdBindIndexBuffer(vulk->commandBuffer, quadIndexBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulk->commandBuffer, 6, 1, 0, 0, 0);
        }
        //vkCmdEndRenderPass(vulk->commandBuffer);
        vkCmdEndRendering(vulk->commandBuffer);
        writeStamp();
    }


    //present(meshRenderTargets.albedoImage);
    //present(computeColorImage);
    present(renderColorFinalImage);
}





int main(int argCount, char **argv)
{
    VulkanComputeTest app;
    if (app.init("Vulkan, compute test", SCREEN_WIDTH, SCREEN_HEIGHT,
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