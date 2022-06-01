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

#include <render/convertrendertarget.h>
#include <render/lightrendersystem.h>
#include <render/meshrendersystem.h>
#include <render/tonemaprendersystem.h>


#include <scene/scene.h>

#include <string.h>

static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 600;

static constexpr int SHADOW_WIDTH = 2048;
static constexpr int SHADOW_HEIGHT = 2048;

static Vec3 getSunDirection(const Camera &camera)
{
    Vec3 sundir[3];
    getDirectionsFromPitchYawRoll(camera.pitch, camera.yaw, 0.0f, sundir[0], sundir[1], sundir[2]);

    return sundir[2];
}

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
    LightRenderSystem lightRenderSystem;
    TonemapRenderSystem tonemapRenderSystem;

    Image renderColorImage;
    Image renderNormalMapColorImage;
    Image renderDepthImage;

    Image renderShadowDepthImage;

    Image renderHdrImage;


    uint32_t indicesCount = 0;

    PodVector<uint32_t> entityIndices;

    CovertRenderTarget convertFromS16{ VK_FORMAT_R16G16B16A16_SNORM };
    Vec2 fontSize{ 8.0f, 12.0f };

    float rotationAmount = 0.0f;

    bool showNormalMap = false;
    bool rotateOn = false;
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
    destroyImage(renderNormalMapColorImage);
    destroyImage(renderHdrImage);
    destroyImage(renderShadowDepthImage);
}



bool VulkanDrawStuff::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;
    // TEMPORARY!
    //glfwSetWindowPos(window, 2000, 100);
    
    convertFromS16.init(ShaderType::ConvertFromRGBAS16);

    if (!meshRenderSystem.init())
        return false;

    if (!scene.init())
        return false;

    if (!lightRenderSystem.init())
        return false;

    if (!tonemapRenderSystem.init())
        return false;

    camera.position = Vec3(0.0f, 4.0f, 5.0f);

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {3.0f, 1.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING}));
    for(float f = -2.5f; f <= 2.5f; f += 1.0f)
        entityIndices.push_back(scene.addGameEntity({
            .transform = {.pos = {f * 5.0f, 1.0f, -2.0f - float(f * 3.0f)}, .scale = {0.1f, 0.1f, 0.1f } },
            .entityType = EntityType::ARROW }));

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {-3.0f, 1.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING }));

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {0.0f, 1.0f, 2.0f } }, .entityType = EntityType::TEST_THING }));

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {0.0f, -0.1f, 0.0f }, .scale = { 10.0f, 1.0f, 10.0f } }, .entityType = EntityType::FLOOR }));


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

    renderShadowDepthImage = createImage(SHADOW_WIDTH, SHADOW_HEIGHT, vulk->depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main shadow target");
   
    sunCamera.pitch = toRadians(60.0f);
    sunCamera.yaw = toRadians(30.0f);


    resized();

    return true;
}


void VulkanDrawStuff::resized()
{
    destroyImage(renderColorImage);
    destroyImage(renderDepthImage);
    destroyImage(renderNormalMapColorImage);
    destroyImage(renderHdrImage);

    // create color and depth images
    renderColorImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height,
        vulk->defaultColorFormat,

        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main color target image");

    renderNormalMapColorImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height,
        VK_FORMAT_R16G16B16A16_SNORM,

        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main normal map target image");
    
    renderHdrImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height,
        VK_FORMAT_R16G16B16A16_SFLOAT,

        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main color HDR");



    renderDepthImage = createImage(
        vulk->swapchain.width, vulk->swapchain.height, vulk->depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main depth target image");

    fontSystem.setRenderTarget(renderColorImage);
    convertFromS16.updateSourceImage(renderNormalMapColorImage, renderColorImage);
    lightRenderSystem.updateReadTargets(renderColorImage, renderNormalMapColorImage, 
        renderDepthImage, renderShadowDepthImage,
        renderHdrImage);
    tonemapRenderSystem.updateReadTargets(renderHdrImage, renderColorImage);
}

void VulkanDrawStuff::logicUpdate()
{
    VulkanApp::logicUpdate();

    MouseState mouseState = getMouseState();

    checkCameraKeypresses(dt, camera);

    if (isPressed(GLFW_KEY_SPACE))
        showNormalMap = !showNormalMap;

    if (isPressed(GLFW_KEY_Z))
        rotateOn = !rotateOn;

    if (isDown(GLFW_KEY_UP))
        sunCamera.pitch -= dt * 1.0f;
    if (isDown(GLFW_KEY_DOWN))
        sunCamera.pitch += dt * 1.0f;
    if (isDown(GLFW_KEY_LEFT))
        sunCamera.yaw += dt * 1.0f;
    if (isDown(GLFW_KEY_RIGHT))
        sunCamera.yaw -= dt * 1.0f;

    Vec3 sundir = getSunDirection(sunCamera);
    while (sunCamera.pitch >= 2.0f * PI) sunCamera.pitch -= 2.0f * PI;
    while (sunCamera.pitch <= 0.0f) sunCamera.pitch += 2.0f * PI;
    while (sunCamera.yaw >= 2.0f * PI) sunCamera.yaw -= 2.0f * PI;
    while (sunCamera.yaw <= 0.0f) sunCamera.yaw += 2.0f * PI;

    Vec2 renderPos = camera.renderCameraInfo(fontSystem, Vec2(10.0f, 10.0f), fontSize);
    char tmpStr[1024];
    snprintf(tmpStr, 1024, "Show normal mode: %s, rotation enabled: %s, rotaion amount: %.2f, use sun camera: %s",
        showNormalMap ? "on" : "off", rotateOn ? "on" : "off", toDegrees(rotationAmount), useSunCamera ? "on" : "off");
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 0.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "SunPitch: %.3f, SunYaw: %.3f, Sundir: %.3f, %.3f, %.3f", 
        toDegrees(sunCamera.pitch), toDegrees(sunCamera.yaw), sundir.x, sundir.y, sundir.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 1.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Sun pos: %.3f, %.3f, %.3f", sunCamera.position.x, sunCamera.position.y, sunCamera.position.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 2.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    meshRenderSystem.clear();
}

void VulkanDrawStuff::renderUpdate()
{
    VulkanApp::renderUpdate();




    for (uint32_t entityIndex : entityIndices)
    {
        GameEntity& entity = scene.getEntity(entityIndex);
        if (entity.entityType == EntityType::NUM_OF_ENTITY_TYPES ||
            entity.entityType == EntityType::FLOOR)
            continue;

        entity.transform.rot = getQuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), rotationAmount);
    }
    if (rotateOn)
    {
        rotationAmount += 1.5f * dt;
        while (rotationAmount >= 2.0f * PI) rotationAmount -= 2.0f * PI;
        while (rotationAmount <= -2.0f * PI) rotationAmount += 2.0f * PI;
    }

    scene.update(getTime());
    meshRenderSystem.prepareToRender();

    Vec3 sundir = getSunDirection(sunCamera);
    lightRenderSystem.setSunDirection(sundir);

    lightRenderSystem.update();
}

void VulkanDrawStuff::renderDraw()
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

    addImageBarrier(imageBarrier(renderShadowDepthImage,
        0, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));
    flushBarriers();

    // Drawingg
    {
        meshRenderSystem.render(renderColorImage, renderNormalMapColorImage, renderDepthImage);
        meshRenderSystem.renderShadows(renderShadowDepthImage);
    }


    writeStamp(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    if (showNormalMap)
    {

        addImageBarrier(imageBarrier(renderColorImage,
            VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL));
        addImageBarrier(imageBarrier(renderNormalMapColorImage,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));

        flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        convertFromS16.render(renderColorImage.width, renderColorImage.height);
        writeStamp(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }
    else
    {
        addImageBarrier(imageBarrier(renderColorImage,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));
        addImageBarrier(imageBarrier(renderNormalMapColorImage,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));
        addImageBarrier(imageBarrier(renderDepthImage,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));
        addImageBarrier(imageBarrier(renderShadowDepthImage,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));

        addImageBarrier(imageBarrier(renderHdrImage,
            0, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL));

        flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);




        lightRenderSystem.render(renderColorImage.width, renderColorImage.height);
        writeStamp(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        addImageBarrier(imageBarrier(renderHdrImage,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL));
        addImageBarrier(imageBarrier(renderColorImage,
            VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL));
        flushBarriers(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        tonemapRenderSystem.render(renderColorImage.width, renderColorImage.height);
        writeStamp(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }

    addImageBarrier(imageBarrier(renderColorImage,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    flushBarriers(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);


    fontSystem.render();
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
            .useVulkanDebugMarkersRenderDoc = true,
            .vsync = VSyncType::IMMEDIATE_NO_VSYNC
        }))
    {
        app.run();
    }

    return 0;
}