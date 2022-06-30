
#include <components/transform_functions.h>

#include <core/camera.h>
#include <core/general.h>
#include <core/glfw_keys.h>
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
#include <render/tonemaprendersystem.h>

#include <render/lightingrendertargets.h>
#include <render/meshrendertargets.h>


#include <scene/scene.h>

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

    uint32_t selectedEntityIndex = ~0u;

    bool showNormalMap = false;

    Vec3 lineFrom;
    Vec3 lineTo;


    uint32_t characterEntityIndex = ~0u;
    uint32_t characterEntityIndex2 = ~0u;

    uint32_t moveAnimationIndex = ~0u;
    float angle = 0.0f;
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

    scene.addGameEntity({ .transform = {.pos = {0.0f, -0.1f, 0.0f }, .scale = { 10.0f, 1.0f, 10.0f } }, .entityType = EntityType::FLOOR });

    characterEntityIndex = scene.addGameEntity({ .transform = { .pos = { 0.0f, 0.0f, 0.0f } }, .entityType = EntityType::NEW_CHARACTER_TEST });
    moveAnimationIndex = scene.addAnimation(characterEntityIndex, "Run", PlayMode::Loop);

    characterEntityIndex2 = scene.addGameEntity({ .transform = { .pos = { 1.5f, 0.0f, 1.5f } }, .entityType = EntityType::NEW_CHARACTER_TEST });
    scene.addAnimation(characterEntityIndex2, "Punch", PlayMode::Loop);

    for(float x = -10.0f; x <= 10.0f; x += 5.0f)
    {
        scene.addGameEntity({ .transform = { .pos = { x, 0.0f, -10.0f } }, .entityType = EntityType::TREE_SMOOTH });
        scene.addGameEntity({ .transform = { .pos = { x, 0.0f, 10.0f } }, .entityType = EntityType::TREE_SMOOTH });
        scene.addGameEntity({ .transform = { .pos = { -10, 0.0f, x } }, .entityType = EntityType::TREE_SMOOTH });
        scene.addGameEntity({ .transform = { .pos = { 10, 0.0f, x } }, .entityType = EntityType::TREE_SMOOTH });
    }

    sunCamera.pitch = toRadians(330.0f);
    sunCamera.yaw = toRadians(60.0f);

    camera.position = Vec3(0.0f, 20.0f, 30.0f);
    camera.fovY = 15.0f;
    return resized();
}


bool VulkanDrawStuff::resized()
{
    if (!meshRenderTargets.resizeMeshTargets())
        return false;
    if (!lightingRenderTargets.resizeLightingTargets())
        return false;

    meshRenderSystem.setRenderTargets(meshRenderTargets);
    lineRenderSystem.setRendertargets(meshRenderTargets.albedoImage, meshRenderTargets.depthImage);
    fontSystem.setRenderTarget(meshRenderTargets.albedoImage);
    convertFromS16.updateSourceImages(meshRenderTargets);

    lightRenderSystem.updateReadTargets(meshRenderTargets, lightingRenderTargets);
    tonemapRenderSystem.updateReadTargets(lightingRenderTargets.lightingTargetImage, meshRenderTargets.albedoImage);

    

    return true;
}



void VulkanDrawStuff::logicUpdate()
{
    VulkanApp::logicUpdate();
    lineRenderSystem.clear();

    MouseState mouseState = getMouseState();

    //checkCameraKeypresses(dt, camera);
    GameEntity &entity = scene.getEntity(characterEntityIndex);
    GameEntity &entity2 = scene.getEntity(characterEntityIndex2);

    float moveSpeed = dt * 5.0f;
    Vec3 dir;
    
    uint32_t animIndex = 0u;
    
    if(isDown(GLFW_KEY_W))
    {
        dir = dir + Vec3(0.0f, 0.0f, -1.0f);
        angle = Pi;
        animIndex = 1u;
    }
    if(isDown(GLFW_KEY_S))
    {
        dir = dir + Vec3(0.0f, 0.0f, 1.0f);
        angle = 0.0f;
        animIndex = 1u;
    }
    if(isDown(GLFW_KEY_A))
    {
        dir = dir + Vec3(-1.0f, 0.0f, 0.0f);
        if(isDown(GLFW_KEY_W))
            angle = Pi * 5.0f / 4.0f;
        else if(isDown(GLFW_KEY_S))
            angle = Pi * 7.0f / 4.0f;
        else
            angle = Pi * 3.0f / 2.0f;

        animIndex = 1u;
    }
    if(isDown(GLFW_KEY_D))
    {
        dir = dir + Vec3(1.0f, 0.0f, 0.0f);
        if(isDown(GLFW_KEY_W))
            angle = Pi * 3.0f / 4.0f;
        else if(isDown(GLFW_KEY_S))
            angle = Pi * 1.0f / 4.0f;
        else
            angle = Pi / 2.0f;

        animIndex = 1u;
    }

    if(isPressed(GLFW_KEY_SPACE))
    {
        scene.addAnimation(characterEntityIndex, "Punch", PlayMode::PlayOnce);
    }
    if(isPressed(GLFW_KEY_X))
    {
        if(moveAnimationIndex != ~0u)
        {
            scene.replaceAnimation(characterEntityIndex, "Run", moveAnimationIndex);
        }
    }
    if(isPressed(GLFW_KEY_Z))
    {
        if(moveAnimationIndex != ~0u)
        {
            scene.replaceAnimation(characterEntityIndex, "walk", moveAnimationIndex);
        }
    }

    if(entity.animationIndex != animIndex)
    {
        entity.animationIndex = animIndex;
        entity.animationTime = 0.0;
    }
    
    entity.transform.pos = entity.transform.pos + dir * moveSpeed;
    Quat newRot = getQuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), angle);
    entity.transform.rot = newRot;

    entity2.transform.pos = entity2.transform.pos + dir * moveSpeed;
    entity2.transform.rot = newRot;
    camera.position = camera.position + dir * moveSpeed;
    camera.lookAt(entity.transform.pos);

    sunCamera.lookAt(entity.transform.pos);
    if (isPressed(GLFW_KEY_P))
        showNormalMap = !showNormalMap;

    Vec3 sundir = getSunDirection(sunCamera);
    while (sunCamera.pitch >= 2.0f * PI) sunCamera.pitch -= 2.0f * PI;
    while (sunCamera.pitch <= 0.0f) sunCamera.pitch += 2.0f * PI;
    while (sunCamera.yaw >= 2.0f * PI) sunCamera.yaw -= 2.0f * PI;
    while (sunCamera.yaw <= 0.0f) sunCamera.yaw += 2.0f * PI;

    Vec2 renderPos = camera.renderCameraInfo(fontSystem, Vec2(10.0f, 10.0f), fontSize);
    char tmpStr[1024];
    snprintf(tmpStr, 1024, "Show normal mode: %s, use sun camera: %s",
        showNormalMap ? "on" : "off", useSunCamera ? "on" : "off");
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 0.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "SunPitch: %.3f, SunYaw: %.3f, Sundir: %.3f, %.3f, %.3f",
        toDegrees(sunCamera.pitch), toDegrees(sunCamera.yaw), sundir.x, sundir.y, sundir.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 1.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Sun pos: %.3f, %.3f, %.3f", sunCamera.position.x, sunCamera.position.y, sunCamera.position.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 2.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    meshRenderSystem.clear();

    if(mouseState.leftButtonDown &&
        mouseState.x >= 0 && mouseState.y >= 0 &&
        mouseState.x < vulk->swapchain.width && mouseState.y < vulk->swapchain.height)
    {
        selectedEntityIndex = ~0u;

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

    scene.update(dt);
    meshRenderSystem.prepareToRender();

    Vec3 sundir = getSunDirection(sunCamera);
    lightRenderSystem.setSunDirection(sundir);

    lightRenderSystem.update();

    lineRenderSystem.prepareToRender();

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

    present(meshRenderTargets.albedoImage);
}

int main(int argCount, char **argv)
{
    VulkanDrawStuff app;
    if (app.init("Game", SCREEN_WIDTH, SCREEN_HEIGHT,
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