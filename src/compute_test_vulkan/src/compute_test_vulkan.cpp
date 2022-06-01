#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/camera.h>
#include <core/general.h>
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
    virtual void resized() override;

    bool createPipelines();

    bool recreateDescriptor();

public:
    Scene scene;
    MeshRenderSystem meshRenderSystem;

    UniformBufferHandle quadHandle;

    Buffer quadIndexBuffer;

    Pipeline computePipeline;
    Pipeline graphicsFinalPipeline;

    Image renderColorImage;
    Image renderNormalMapColorImage;
    Image renderDepthImage;

    Image computeColorImage;
    VkSampler textureSampler = nullptr;

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

    destroyImage(renderColorImage);
    destroyImage(renderNormalMapColorImage);
    destroyImage(renderDepthImage);

    destroyImage(computeColorImage);
    destroyImage(renderColorFinalImage);

    destroySampler(textureSampler);
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
        flushBarriers(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        endSingleTimeCommands();
    }
    camera.position = Vec3(0.0f, 4.0f, 5.0f);

    if (!scene.init())
        return false;
    scene.addGameEntity({ .transform = {.pos = {3.0f, 0.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING });

    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;

    textureSampler = createSampler(samplerInfo);
    if (!textureSampler)
    {
        printf("Failed to create sampler for font rendering");
        return false;
    }

    return createPipelines();
}


bool VulkanComputeTest::createPipelines()
{
    {
        Pipeline &pipeline = computePipeline;
        if(!createComputePipeline(getShader(ShaderType::ComputeTestComp), pipeline, "Compute copy pipeline test"))
        {
            printf("Failed to create compute pipeline!\n");
            return false;
        }

    }

    // Create copy-pipelines
    {
        Pipeline &pipeline = graphicsFinalPipeline;
        if (!createGraphicsPipeline(
            getShader(ShaderType::TexturedQuadVert), getShader(ShaderType::TexturedQuadFrag),
            { RenderTarget{.format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE } },
            {}, pipeline, "Graphics copy pipeline test", true))
        {
            printf("Failed to create graphics pipeline\n");
            return false;
        }

    }

    resized();
    return recreateDescriptor();
}

bool VulkanComputeTest::recreateDescriptor()
{
    if(!textureSampler)
        return false;

    {
        Pipeline &pipeline = computePipeline;
        destroyDescriptor(pipeline.descriptor);
        pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
            {
                DescriptorInfo(vulk->renderFrameBufferHandle),
                //DescriptorInfo(renderColorImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureSampler),
                DescriptorInfo(renderColorImage.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
                DescriptorInfo(computeColorImage.imageView, VK_IMAGE_LAYOUT_GENERAL, nullptr),
            });
        if (!createDescriptor(pipeline))
        {
            printf("Failed to create compute pipeline descriptor\n");
            return false;
        }
        if (!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }
    {
        Pipeline &pipeline = graphicsFinalPipeline;
        destroyDescriptor(pipeline.descriptor);
        pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
        {
            DescriptorInfo(vulk->renderFrameBufferHandle),
            DescriptorInfo(quadHandle),
            DescriptorInfo(computeColorImage.imageView, VK_IMAGE_LAYOUT_GENERAL, textureSampler),
        });

        if (!createDescriptor(pipeline))
        {
            printf("Failed to create graphics pipeline descriptor\n");
            return false;
        }

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
    destroyImage(renderNormalMapColorImage);
    destroyImage(renderDepthImage);
    destroyImage(computeColorImage);
    destroyImage(renderColorFinalImage);

    // create color and depth images
    renderColorImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height,
        vulk->defaultColorFormat,

        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main color target image");

    renderNormalMapColorImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height,
        VK_FORMAT_R16G16B16A16_SNORM,

        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main normal map target image");

    renderDepthImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height, vulk->depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main depth target image");

    computeColorImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height,
        vulk->defaultColorFormat,

        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Compute target image");

    renderColorFinalImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height,
        vulk->defaultColorFormat,

        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Render color final image");

    fontSystem.setRenderTarget(renderColorImage);
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
    scene.update(getTime());
    meshRenderSystem.prepareToRender();
}

void VulkanComputeTest::renderDraw()
{
    const SwapChain& swapchain = vulk->swapchain;

    addImageBarrier(imageBarrier(renderColorImage,
        0, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

    addImageBarrier(imageBarrier(renderNormalMapColorImage,
        0, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

    addImageBarrier(imageBarrier(renderDepthImage,
        0, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));

    flushBarriers();

    // Drawingg
    {
        meshRenderSystem.render(renderColorImage, renderNormalMapColorImage, renderDepthImage);
        fontSystem.render();
    }

    writeStamp(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    {
        addImageBarrier(imageBarrier(renderColorImage,
            //VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));

        addImageBarrier(imageBarrier(computeColorImage,
            0, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL));

        flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);


        bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdDispatch(vulk->commandBuffer, (swapchain.width + 7) / 8, (swapchain.height + 7) / 8, 1);

    }
    writeStamp(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    {
        addImageBarrier(imageBarrier(renderColorFinalImage,
            0, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

        addImageBarrier(imageBarrier(computeColorImage,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));

        flushBarriers(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);


        //beginRenderPass(graphicsFinalPipeline, {});
        beginRendering({ RenderImage{ .image = &renderColorFinalImage, .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE } }, {});
        insertDebugRegion("RenderCopy", Vec4(1.0f, 0.0f, 0.0f, 1.0f));

        // draw calls here
        // Render
        {
            bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsFinalPipeline);
            vkCmdBindIndexBuffer(vulk->commandBuffer, quadIndexBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulk->commandBuffer, 6, 1, 0, 0, 0);
        }
        //vkCmdEndRenderPass(vulk->commandBuffer);
        vkCmdEndRendering(vulk->commandBuffer);

    }

    writeStamp(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    //present(renderColorImage);
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