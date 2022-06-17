#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <components/transform_functions.h>

#include <core/camera.h>
#include <core/general.h>
#include <core/json.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>

#include <container/arraysliceview.h>

#include <gui/componentviews.h>

#include <math/general_math.h>
#include <math/hitpoint.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/ray.h>
#include <math/vector3.h>
#include <math/quaternion_inline_functions.h>

#include <model/gltf.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>


#include <render/convertrendertarget.h>
#include <render/lightrendersystem.h>
#include <render/linerendersystem.h>
#include <render/meshrendersystem.h>
#include <render/myimgui.h>
#include <render/tonemaprendersystem.h>

#include <render/lightingrendertargets.h>
#include <render/meshrendertargets.h>


#include <scene/scene.h>

#include <imgui/imgui.h>
#include <string.h>

static constexpr int SCREEN_WIDTH = 1024;
static constexpr int SCREEN_HEIGHT = 768;

static constexpr int SHADOW_WIDTH = 2048;
static constexpr int SHADOW_HEIGHT = 2048;

static Vec3 getSunDirection(const Camera &camera)
{
    Vec3 sundir[3];
    getDirectionsFromPitchYawRoll(camera.pitch, camera.yaw, 0.0f, sundir[0], sundir[1], sundir[2]);

    return -sundir[2];
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
    virtual bool resized() override;

public:
    Scene scene;
    MeshRenderSystem meshRenderSystem;
    LightRenderSystem lightRenderSystem;
    TonemapRenderSystem tonemapRenderSystem;

    LightingRenderTargets lightingRenderTargets;
    MeshRenderTargets meshRenderTargets;
    LineRenderSystem lineRenderSystem;

    ConvertRenderTarget convertFromS16{ VK_FORMAT_R16G16B16A16_SNORM };
    Vec2 fontSize{ 8.0f, 12.0f };

    MyImgui imgui;

    uint32_t selectedEntityIndex = ~0u;
    float rotationAmount = 0.0f;

    bool showNormalMap = false;
    bool rotateOn = false;
    bool mouseHover = false;

    Vec3 lineFrom;
    Vec3 lineTo;
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanDrawStuff::~VulkanDrawStuff()
{
}



bool VulkanDrawStuff::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;
    // TEMPORARY!
    //glfwSetWindowPos(window, 2000, 100);

    if (!imgui.init(window))
        return false;

    if (!convertFromS16.init(ShaderType::ConvertFromRGBAS16))
        return false;

    if (!meshRenderSystem.init())
        return false;

    if (!scene.init())
        return false;

    if (!lightRenderSystem.init())
        return false;

    if (!tonemapRenderSystem.init())
        return false;

    if (!meshRenderTargets.resizeShadowTarget(SHADOW_WIDTH, SHADOW_HEIGHT))
        return false;

    if (!lineRenderSystem.init())
        return false;
    camera.position = Vec3(0.0f, 4.0f, 5.0f);

    scene.addGameEntity({ .transform = {.pos = {0.0f, -0.1f, 0.0f }, .scale = { 10.0f, 1.0f, 10.0f } }, .entityType = EntityType::FLOOR });

    scene.addGameEntity({ .transform = {.pos = {3.0f, 1.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING});
    scene.addGameEntity({ .transform = {.pos = {-3.0f, 1.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING });

    scene.addGameEntity({ .transform = {.pos = {0.0f, 0.1f, -15.0f } }, .entityType = EntityType::CHARACTER });

    scene.addGameEntity({ .transform = {.pos = {-3.0f, 0.1f, -20.0f } }, .entityType = EntityType::LOW_POLY_CHAR });
    scene.addGameEntity({ .transform = {.pos = {3.0f, 0.1f, -20.0f } }, .entityType = EntityType::LOW_POLY_CHAR });

    for(int y = -10; y < 10; ++y)
    {
        for(int x = -150; x < 150; ++x)
        {
            Vec3 pos(x, 0.0f, -50.0f + y);
            pos.x += 0.5f;
            pos.y += 0.5f;

            scene.addGameEntity({ .transform = { .pos = pos }, .entityType = EntityType::LOW_POLY_CHAR });
        }
    }


    for (float f = -2.5f; f <= 2.5f; f += 1.0f)
    {
        scene.addGameEntity({.transform = {.pos = {f * 5.0f, 1.0f, -5.0f}, .scale = {0.1f, 0.1f, 0.1f } }, .entityType = EntityType::ARROW });
        scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 10.0f}, }, .entityType = EntityType::TREE });
        scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 15.0f}, }, .entityType = EntityType::TREE_SMOOTH });
        scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 20.0f}, }, .entityType = EntityType::BLOB });
        scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 25.0f}, }, .entityType = EntityType::BLOB_FLAT });
    }
    scene.addGameEntity({ .transform = {.pos = {0.0f, 1.0f, 2.0f } }, .entityType = EntityType::TEST_THING });



    sunCamera.pitch = toRadians(330.0f);
    sunCamera.yaw = toRadians(30.0f);

    return resized();
}


bool VulkanDrawStuff::resized()
{
    if (!meshRenderTargets.resizeMeshTargets())
        return false;
    if (!lightingRenderTargets.resizeLightingTargets())
        return false;

    fontSystem.setRenderTarget(meshRenderTargets.albedoImage);
    convertFromS16.updateSourceImages(meshRenderTargets);

    lightRenderSystem.updateReadTargets(meshRenderTargets, lightingRenderTargets);
    tonemapRenderSystem.updateReadTargets(lightingRenderTargets.lightingTargetImage, meshRenderTargets.albedoImage);

    imgui.updateRenderTarget(meshRenderTargets.albedoImage);
    return true;
}



void VulkanDrawStuff::logicUpdate()
{
    VulkanApp::logicUpdate();
    static uint32_t counter = 0;
    if(counter++ >= 100)
    {
        //printf("Matrix time: %f\n", float(getMatrixTime() / counter));
        //printf("Bytebuffer time: %f\n", float(getByteBufferTimer() / counter));
        counter = 0;
    }
    lineRenderSystem.clear();

    MouseState mouseState = getMouseState();

    checkCameraKeypresses(dt, camera);

    if (isPressed(GLFW_KEY_KP_ADD))
    {
        for (auto &entity : scene.getEntities())
        {
            ++entity.animationIndex;
        }
    }
    if (isPressed(GLFW_KEY_KP_SUBTRACT))
    {
        for(auto &entity : scene.getEntities())
        {
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

    if(mouseState.leftButtonDown && !mouseHover)
        selectedEntityIndex = ~0u;


    if(mouseState.leftButtonDown && !mouseHover &&
        mouseState.x >= 0 && mouseState.y >= 0 &&
        mouseState.x < vulk->swapchain.width && mouseState.y < vulk->swapchain.height)
    {
        Vec2 coord = Vec2(mouseState.x, mouseState.y);
        Ray ray{ Uninit };
        if(useSunCamera)
            ray = sunCamera.getRayFromScreenPixelCoordinates(coord, getWindowSize());
        else
            ray = camera.getRayFromScreenPixelCoordinates(coord, getWindowSize());

        HitPoint hitpoint{ Uninit };
        selectedEntityIndex = scene.castRay(ray, hitpoint);
        if(selectedEntityIndex != ~0u)
        {
            lineTo = lineTo = hitpoint.point;
            lineFrom = ray.pos;
        }
    }

    lineRenderSystem.addLine(lineFrom, lineTo, getColor(0.0f, 1.0f, 0.0f, 1.0f));
}

void VulkanDrawStuff::renderUpdate()
{
    VulkanApp::renderUpdate();
    imgui.renderBegin();

    uint32_t grayColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.75f, 0.75f, 0.75f, 1.0f));
    uint32_t selectedColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.75f, 0.75f, 0.75f, 1.0f));


    uint32_t unSelectedRedColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    uint32_t unSelectedGreenColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    uint32_t unSelectedBlueColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.0f, 0.0f, 1.0f, 1.0f));


    uint32_t selectedRedColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    uint32_t selectedGreenColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    uint32_t selectedBlueColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.0f, 0.0f, 1.0f, 1.0f));


    for (auto &entity : scene.getEntities())
    {
        Vec4 linePoints4[8];
        Vec3 linePoints[8];

        Mat3x4 m = getModelMatrix(entity.transform);
        const auto &bounds = scene.getBounds(entity.index);
        const auto &bmin = bounds.min;
        const auto &bmax = bounds.max;

        linePoints4[0] = mul(m, Vec4(bmin.x, bmin.y, bmin.z, 1.0f));
        linePoints4[1] = mul(m, Vec4(bmax.x, bmin.y, bmin.z, 1.0f));
        linePoints4[2] = mul(m, Vec4(bmin.x, bmax.y, bmin.z, 1.0f));
        linePoints4[3] = mul(m, Vec4(bmax.x, bmax.y, bmin.z, 1.0f));
        linePoints4[4] = mul(m, Vec4(bmin.x, bmin.y, bmax.z, 1.0f));
        linePoints4[5] = mul(m, Vec4(bmax.x, bmin.y, bmax.z, 1.0f));
        linePoints4[6] = mul(m, Vec4(bmin.x, bmax.y, bmax.z, 1.0f));
        linePoints4[7] = mul(m, Vec4(bmax.x, bmax.y, bmax.z, 1.0f));

        for(uint32_t i = 0; i < 8; ++i)
            linePoints[i] = Vec3(linePoints4[i].x, linePoints4[i].y, linePoints4[i].z);

        Vec4 multip(0.5f, 0.5f, 0.5f, 1.0f);
        uint32_t drawColor = selectedEntityIndex == entity.index ? selectedColor : grayColor;
        lineRenderSystem.addLine(linePoints[1], linePoints[3], drawColor);
        lineRenderSystem.addLine(linePoints[2], linePoints[3], drawColor);
        lineRenderSystem.addLine(linePoints[1], linePoints[5], drawColor);
        lineRenderSystem.addLine(linePoints[2], linePoints[6], drawColor);
        lineRenderSystem.addLine(linePoints[3], linePoints[7], drawColor);
        lineRenderSystem.addLine(linePoints[4], linePoints[5], drawColor);
        lineRenderSystem.addLine(linePoints[4], linePoints[6], drawColor);
        lineRenderSystem.addLine(linePoints[5], linePoints[7], drawColor);
        lineRenderSystem.addLine(linePoints[6], linePoints[7], drawColor);
        
        uint32_t redColor = selectedEntityIndex == entity.index ? selectedRedColor : unSelectedRedColor;
        uint32_t greenColor = selectedEntityIndex == entity.index ? selectedGreenColor : unSelectedGreenColor;
        uint32_t blueColor = selectedEntityIndex == entity.index ? selectedBlueColor : unSelectedBlueColor;

        lineRenderSystem.addLine(linePoints[0], linePoints[1], redColor);
        lineRenderSystem.addLine(linePoints[0], linePoints[2], greenColor);
        lineRenderSystem.addLine(linePoints[0], linePoints[4], blueColor);

        if (entity.entityType == EntityType::NUM_OF_ENTITY_TYPES ||
            entity.entityType == EntityType::FLOOR)
            continue;

        entity.transform.rot = getQuaternionFromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), rotationAmount);
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

    lineRenderSystem.prepareToRender();

    if(selectedEntityIndex != ~0u)
    {
        mouseHover = drawEntity("Selected entity", scene.getEntity(selectedEntityIndex));
    }
    else
    {
        mouseHover = false;
    }

}

void VulkanDrawStuff::renderDraw()
{
    meshRenderTargets.prepareTargetsForMeshRendering();
    meshRenderTargets.prepareTargetsForShadowRendering();
    // Drawingg
    {
        meshRenderSystem.render(meshRenderTargets);
        meshRenderSystem.renderShadows(meshRenderTargets);
    }

    if (showNormalMap)
    {
        prepareToComputeSampleRead(meshRenderTargets.normalMapImage);

        prepareToComputeImageWrite(meshRenderTargets.albedoImage);

        convertFromS16.render(meshRenderTargets.albedoImage.width, meshRenderTargets.albedoImage.height);

    }
    else
    {
        {
            meshRenderTargets.prepareTargetsForLightingComputeSampling();
            lightingRenderTargets.prepareTargetsForLightingComputeWriting();

            lightRenderSystem.render(lightingRenderTargets.lightingTargetImage.width, lightingRenderTargets.lightingTargetImage.height);
        }

        {
            lightingRenderTargets.prepareForTonemapSampling();

            prepareToComputeImageWrite(meshRenderTargets.albedoImage);

            tonemapRenderSystem.render(meshRenderTargets.albedoImage.width, meshRenderTargets.albedoImage.height);

        }
    }

    prepareToGraphicsSampleWrite(meshRenderTargets.albedoImage);
    {
        prepareToGraphicsSampleWrite(meshRenderTargets.depthImage);
        lineRenderSystem.render(meshRenderTargets.albedoImage, meshRenderTargets.depthImage);
    }

    {
        fontSystem.render();
    }

    imgui.render();

    present(meshRenderTargets.albedoImage);
}

int main(int argCount, char **argv)
{
    VulkanDrawStuff app;
    if (app.init("Vulkan, draw models", SCREEN_WIDTH, SCREEN_HEIGHT,
        {
            .showInfoMessages = false,
            .useHDR = false,
            .useIntegratedGpu = false,
            .useValidationLayers = true,
            .useVulkanDebugMarkersRenderDoc = true,
            .vsync = VSyncType::IMMEDIATE_NO_VSYNC
        }))
    {
        app.run();
    }

    return 0;
}