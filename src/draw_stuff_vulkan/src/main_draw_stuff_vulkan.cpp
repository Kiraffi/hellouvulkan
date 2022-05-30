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

#include <render/meshsystem.h>

#include <scene/scene.h>

#include <string.h>

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 600;



class VulkanDrawStuff : public VulkanApp
{
public:
    VulkanDrawStuff() : scene(meshRenderSystem) { }
    virtual ~VulkanDrawStuff() override;
    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual void resized() override;

public:
    Scene scene;
    MeshRenderSystem meshRenderSystem;

    Camera camera;

    UniformBufferHandle uniformDataHandle;

    Image renderColorImage;
    Image renderDepthImage;

    uint32_t indicesCount = 0;

    PodVector<uint32_t> entityIndices;
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanDrawStuff::~VulkanDrawStuff()
{
    destroyImage(renderColorImage);
    destroyImage(renderDepthImage);
}



bool VulkanDrawStuff::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;


    uniformDataHandle = vulk->uniformBufferManager.reserveHandle();
    if (!uniformDataHandle.isValid())
        return false;

    if (!meshRenderSystem.init(uniformDataHandle))
        return false;

    if (!scene.init())
        return false;

    camera.position = Vec3(0.0f, 4.0f, 5.0f);

    resized();

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {3.0f, 0.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING}));
    for(float f = -2.5f; f <= 2.5f; f += 1.0f)
        entityIndices.push_back(scene.addGameEntity({
            .transform = {.pos = {f * 5.0f, 0.0f, -2.0f - float(f * 3.0f)}, .scale = {0.1f, 0.1f, 0.1f } },
            .entityType = EntityType::ARROW }));

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {-3.0f, 0.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING }));

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {0.0f, 0.0f, 2.0f } }, .entityType = EntityType::TEST_THING }));


    for (float f = -2.5f; f <= 2.5f; f += 1.0f)
        entityIndices.push_back(scene.addGameEntity({
            .transform = {.pos = {f * 5.0f, 0.0f, 10.0f}, },
            .entityType = EntityType::TREE }));

    for (float f = -2.5f; f <= 2.5f; f += 1.0f)
        entityIndices.push_back(scene.addGameEntity({
            .transform = {.pos = {f * 5.0f, 0.0f, 15.0f}, },
            .entityType = EntityType::TREE_SMOOTH }));


    for (float f = -2.5f; f <= 2.5f; f += 1.0f)
        entityIndices.push_back(scene.addGameEntity({
            .transform = {.pos = {f * 5.0f, 0.0f, 20.0f}, },
            .entityType = EntityType::BLOB }));

    for (float f = -2.5f; f <= 2.5f; f += 1.0f)
        entityIndices.push_back(scene.addGameEntity({
            .transform = {.pos = {f * 5.0f, 0.0f, 25.0f}, },
            .entityType = EntityType::BLOB_FLAT }));

    return true;
}


void VulkanDrawStuff::resized()
{
    destroyImage(renderColorImage);
    destroyImage(renderDepthImage);

    // create color and depth images
    renderColorImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height,
        vulk->defaultColorFormat,

        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main color target image");

    renderDepthImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height, vulk->depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main depth target image");

    fontSystem.setRenderTarget(renderColorImage);
}

void VulkanDrawStuff::logicUpdate()
{
    VulkanApp::logicUpdate();

    MouseState mouseState = getMouseState();

    checkCameraKeypresses(dt, camera);

    camera.renderCameraInfo(fontSystem, Vec2(10.0f, 10.0f), Vec2(8.0f, 12.0f));
    meshRenderSystem.clear();
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

    const SwapChain &swapchain = vulk->swapchain;
    camera.aspectRatioWByH = float(swapchain.width) / float(swapchain.height);
    camera.fovY = 90.0f;
    camera.zFar = 200.0f;
    camera.zNear = 0.001f;

    frameBufferData.viewProj = camera.perspectiveProjection();
    frameBufferData.mvp = frameBufferData.viewProj * frameBufferData.camMat;

    static float rotationAmount = Pi * 0.25f;
    for(uint32_t entityIndex : entityIndices)
    {
        GameEntity& entity = scene.getEntity(entityIndex);
        if (entity.entityType != EntityType::NUM_OF_ENTITY_TYPES)
        {
            entity.transform.rot = getQuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), rotationAmount);
        }
    }
    rotationAmount += 1.5f * dt;

    addToCopylist(frameBufferData, uniformDataHandle);



    scene.update(getTime());
    meshRenderSystem.prepareToRender();
}

void VulkanDrawStuff::renderDraw()
{
    const SwapChain& swapchain = vulk->swapchain;

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
        meshRenderSystem.render(renderColorImage, renderDepthImage);
        fontSystem.render();
    }

    writeStamp(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    present(renderColorImage);
}





int main(int argCount, char **argv)
{
    VulkanDrawStuff app;
    if (app.init("Vulkan, draw models", SCREEN_WIDTH, SCREEN_HEIGHT,
        {
            .showInfoMessages = false,
            .useHDR = false,
            .useIntegratedGpu = true,
            .useValidationLayers = true,
            .useVulkanDebugMarkersRenderDoc = false,
            .vsync = VSyncType::FIFO_VSYNC
        }))
    {
        app.run();
    }

    return 0;
}