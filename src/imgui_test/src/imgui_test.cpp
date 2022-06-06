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
#include <render/myimgui.h>
#include <render/tonemaprendersystem.h>

#include <gui/examplegui.h>

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

class ImguiTest : public VulkanApp
{
public:
    ImguiTest() : scene(meshRenderSystem) { }
    virtual ~ImguiTest() override;
    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

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

    PodVector<uint32_t> entityIndices;

    CovertRenderTarget convertFromS16{ VK_FORMAT_R16G16B16A16_SNORM };
    Vec2 fontSize{ 8.0f, 12.0f };

    MyImgui imgui;

    float rotationAmount = 0.0f;

    bool showNormalMap = false;
    bool rotateOn = false;
};


////////////////////////
//
// DEINIT
//
////////////////////////

ImguiTest::~ImguiTest()
{
    destroyImage(renderColorImage);
    destroyImage(renderDepthImage);
    destroyImage(renderNormalMapColorImage);
    destroyImage(renderHdrImage);
    destroyImage(renderShadowDepthImage);
}



bool ImguiTest::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;
    // TEMPORARY!
    //glfwSetWindowPos(window, 2000, 100);
    
    if (!imgui.init(window))
        return false;

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

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {0.0f, -0.1f, 0.0f }, .scale = { 10.0f, 1.0f, 10.0f } }, .entityType = EntityType::FLOOR }));
    
 
    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {0.0f, 0.1f, 0.0f } }, .entityType = EntityType::LOW_POLY_CHAR }));


    if (!createRenderTargetImage(SHADOW_WIDTH, SHADOW_HEIGHT, vulk->depthFormat,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        "Main shadow target", renderShadowDepthImage))
    {
        printf("Failed to create shadow target image\n");
        return false;
    }
   
    sunCamera.pitch = toRadians(60.0f);
    sunCamera.yaw = toRadians(30.0f);

    return resized();
}


bool ImguiTest::resized()
{
    uint32_t width = vulk->swapchain.width;
    uint32_t height = vulk->swapchain.height;

    // create color and depth images
    if (!createRenderTargetImage(width, height, vulk->defaultColorFormat,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            "Main color target image", renderColorImage))
    {
        printf("Failed to create %s\n", renderColorImage.imageName);
        return false;
    }

    if (!createRenderTargetImage(width, height, VK_FORMAT_R16G16B16A16_SNORM,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, //VK_IMAGE_USAGE_STORAGE_BIT,
            "Main normal map target image", renderNormalMapColorImage))
    {
        printf("Failed to create %s\n", renderNormalMapColorImage.imageName);
        return false;
    }

    if (!createRenderTargetImage(width, height, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Main color HDR", renderHdrImage))
    {
        printf("Failed to create %s\n", renderHdrImage.imageName);
        return false;
    }

    if(!createRenderTargetImage(width, height, vulk->depthFormat,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        "Main depth target image", renderDepthImage))
    {
        printf("Failed to create %s\n", renderDepthImage.imageName);
        return false;
    }

    fontSystem.setRenderTarget(renderColorImage);
    convertFromS16.updateSourceImage(renderNormalMapColorImage, renderColorImage);
    lightRenderSystem.updateReadTargets(renderColorImage, renderNormalMapColorImage, 
        renderDepthImage, renderShadowDepthImage,
        renderHdrImage);
    tonemapRenderSystem.updateReadTargets(renderHdrImage, renderColorImage);

    imgui.updateRenderTarget(renderColorImage);
    return true;
}

void ImguiTest::logicUpdate()
{
    VulkanApp::logicUpdate();

    MouseState mouseState = getMouseState();

    checkCameraKeypresses(dt, camera);

    if (isPressed(GLFW_KEY_KP_ADD))
    {
        for (uint32_t entityIndex : entityIndices)
        {
            GameEntity& entity = scene.getEntity(entityIndex);
            ++entity.animationIndex;
        }
    }
    if (isPressed(GLFW_KEY_KP_SUBTRACT))
    {
        for (uint32_t entityIndex : entityIndices)
        {
            GameEntity& entity = scene.getEntity(entityIndex);
            if(entity.animationIndex > 0)
                --entity.animationIndex;
        }
    }

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

void ImguiTest::renderUpdate()
{
    VulkanApp::renderUpdate();
    imgui.renderBegin();

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

    scene.update(dt);
    meshRenderSystem.prepareToRender();

    Vec3 sundir = getSunDirection(sunCamera);
    lightRenderSystem.setSunDirection(sundir);

    lightRenderSystem.update();
    drawExampleGui();


}

void ImguiTest::renderDraw()
{
    prepareToGraphicsSampleWrite(renderColorImage);
    prepareToGraphicsSampleWrite(renderNormalMapColorImage);
    prepareToGraphicsSampleWrite(renderDepthImage);
    prepareToGraphicsSampleWrite(renderShadowDepthImage);

    flushBarriers();

    // Drawingg
    {
        meshRenderSystem.render(renderColorImage, renderNormalMapColorImage, renderDepthImage);
        meshRenderSystem.renderShadows(renderShadowDepthImage);
    }


    writeStamp(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    if (showNormalMap)
    {
        prepareToComputeSampleRead(renderNormalMapColorImage);

        prepareToComputeImageWrite(renderColorImage);

        flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        convertFromS16.render(renderColorImage.width, renderColorImage.height);
        writeStamp(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    }
    else
    {
        {
            prepareToComputeSampleRead(renderColorImage);
            prepareToComputeSampleRead(renderNormalMapColorImage);
            prepareToComputeSampleRead(renderDepthImage);
            prepareToComputeSampleRead(renderShadowDepthImage);

            prepareToComputeImageWrite(renderHdrImage);

            flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

            lightRenderSystem.render(renderColorImage.width, renderColorImage.height);
            writeStamp(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }

        {
            prepareToComputeSampleRead(renderHdrImage);

            prepareToComputeImageWrite(renderColorImage);

            flushBarriers(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

            tonemapRenderSystem.render(renderColorImage.width, renderColorImage.height);
            writeStamp(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
        }
    }

    {
        prepareToGraphicsSampleWrite(renderColorImage);
        flushBarriers(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

        fontSystem.render();
        writeStamp(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
    }
    imgui.render();
    present(renderColorImage);
}

int main(int argCount, char **argv)
{
    ImguiTest app;
    if (app.init("Imguitest", SCREEN_WIDTH, SCREEN_HEIGHT,
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