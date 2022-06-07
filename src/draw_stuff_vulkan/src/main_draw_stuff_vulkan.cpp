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
#include <math/ray.h>
#include <math/vector3.h>

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
    virtual bool resized() override;

public:
    Scene scene;
    MeshRenderSystem meshRenderSystem;
    LightRenderSystem lightRenderSystem;
    TonemapRenderSystem tonemapRenderSystem;

    LightingRenderTargets lightingRenderTargets;
    MeshRenderTargets meshRenderTargets;
    LineRenderSystem lineRenderSystem;

    PodVector<uint32_t> entityIndices;

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

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {0.0f, -0.1f, 0.0f }, .scale = { 10.0f, 1.0f, 10.0f } }, .entityType = EntityType::FLOOR }));

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {3.0f, 1.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING}));
    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {-3.0f, 1.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING }));

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {0.0f, 0.1f, -15.0f } }, .entityType = EntityType::CHARACTER }));

    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {-3.0f, 0.1f, -20.0f } }, .entityType = EntityType::LOW_POLY_CHAR }));
    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {3.0f, 0.1f, -20.0f } }, .entityType = EntityType::LOW_POLY_CHAR }));


    for (float f = -2.5f; f <= 2.5f; f += 1.0f)
    {
        entityIndices.push_back(scene.addGameEntity({
            .transform = {.pos = {f * 5.0f, 1.0f, -5.0f}, .scale = {0.1f, 0.1f, 0.1f } }, .entityType = EntityType::ARROW }));
        entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 10.0f}, }, .entityType = EntityType::TREE }));
        entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 15.0f}, }, .entityType = EntityType::TREE_SMOOTH }));
        entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 20.0f}, }, .entityType = EntityType::BLOB }));
        entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 25.0f}, }, .entityType = EntityType::BLOB_FLAT }));
    }
    entityIndices.push_back(scene.addGameEntity({ .transform = {.pos = {0.0f, 1.0f, 2.0f } }, .entityType = EntityType::TEST_THING }));



    sunCamera.pitch = toRadians(60.0f);
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

    lineRenderSystem.clear();

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

    if(mouseState.leftButtonDown && !mouseHover)
        selectedEntityIndex = ~0u;


    if(mouseState.leftButtonDown && !mouseHover &&
        mouseState.x >= 0 && mouseState.y >= 0 &&
        mouseState.x < vulk->swapchain.width && mouseState.y < vulk->swapchain.height)
    {
        // Calculate the click as ndc. Half pixel offset as rendering.
        Vec2 coord(mouseState.x, mouseState.y);
        coord = (coord + 0.5f) / Vec2((vulk->swapchain.width), (vulk->swapchain.height));
        coord = coord * 2.0f - 1.0f;
        coord.y = -coord.y;

        Matrix mat = inverse(camera.perspectiveProjection() * camera.getCameraMatrix());

        Vec4 rayDir4 = mul(mat, Vec4(coord.x, coord.y, 1.0f, 1.0f));
        rayDir4 = rayDir4 / rayDir4.w;
        Vec3 rayDir(rayDir4.x, rayDir4.y, rayDir4.z);
        rayDir = normalize(rayDir - camera.position);

        lineFrom = camera.position;
        lineTo = camera.position + rayDir * 150.0f;

        Vec3 onePerRayDir = 1.0f / rayDir;

        Ray ray{ .pos = camera.position, .dir = rayDir };

        float closestDist = FLT_MAX;
        for(uint32_t index : entityIndices)
        {
            const auto &entity = scene.getEntity(index);
            Hitpoint hitpoint;
            /*
            if(raySphereIntersect(ray, Sphere{ .pos = entity.transform.pos, .radius = 2.0f }, hitpoint))
            {
                float dist = sqrLen(hitpoint.point - ray.pos);
                if(dist < closestDist)
                {
                    selectedEntityIndex = index;
                    closestDist = dist;
                    lineTo = hitpoint.point;
                }
            }
            */
            if(rayOOBBBoundsIntersect(ray, entity.bounds, entity.transform, hitpoint))
            {
                float dist = sqrLen(hitpoint.point - ray.pos);
                if(dist < closestDist)
                {
                    selectedEntityIndex = index;
                    closestDist = dist;
                    lineTo = hitpoint.point;
                }
            }
        }
    }

    lineRenderSystem.addLine(lineFrom, lineTo, getColor(0.0f, 1.0f, 0.0f, 1.0f));
}

static bool drawEntityImgui(GameEntity &entity, uint32_t index)
{
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Entity");
    ImGui::Text("Type: %u, index: %u", entity.entityType, index);

    // Forcing delesect item, if selected item changes.
    ImGui::PushID(index);
    {
        ImGui::InputFloat3("Pos", &entity.transform.pos.x);
        ImGui::InputFloat4("Rot", &entity.transform.rot.v.x);
        ImGui::InputFloat3("Scale", &entity.transform.scale.x);
    }
    ImGui::PopID();
    bool mouseHover = ImGui::IsWindowHovered(
        ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);// | ImGui::IsAnyItemHovered();
    ImGui::End();

    return mouseHover;
}

void VulkanDrawStuff::renderUpdate()
{
    VulkanApp::renderUpdate();
    imgui.renderBegin();

    for (uint32_t entityIndex : entityIndices)
    {
        GameEntity& entity = scene.getEntity(entityIndex);

        Vec4 linePoints4[8];
        Vec3 linePoints[8];
        Matrix m = getModelMatrix(entity.transform);
        const auto &bmin = entity.bounds.min;
        const auto &bmax = entity.bounds.max;

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
        if(selectedEntityIndex == entityIndex)
            multip = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        lineRenderSystem.addLine(linePoints[1], linePoints[3], getColor(multip * Vec4(0.75f, 0.75f, 0.75f, 1.0f)));
        lineRenderSystem.addLine(linePoints[2], linePoints[3], getColor(multip * Vec4(0.75f, 0.75f, 0.75f, 1.0f)));
        lineRenderSystem.addLine(linePoints[1], linePoints[5], getColor(multip * Vec4(0.75f, 0.75f, 0.75f, 1.0f)));
        lineRenderSystem.addLine(linePoints[2], linePoints[6], getColor(multip * Vec4(0.75f, 0.75f, 0.75f, 1.0f)));
        lineRenderSystem.addLine(linePoints[3], linePoints[7], getColor(multip * Vec4(0.75f, 0.75f, 0.75f, 1.0f)));
        lineRenderSystem.addLine(linePoints[4], linePoints[5], getColor(multip * Vec4(0.75f, 0.75f, 0.75f, 1.0f)));
        lineRenderSystem.addLine(linePoints[4], linePoints[6], getColor(multip * Vec4(0.75f, 0.75f, 0.75f, 1.0f)));
        lineRenderSystem.addLine(linePoints[5], linePoints[7], getColor(multip * Vec4(0.75f, 0.75f, 0.75f, 1.0f)));
        lineRenderSystem.addLine(linePoints[6], linePoints[7], getColor(multip * Vec4(0.75f, 0.75f, 0.75f, 1.0f)));


        lineRenderSystem.addLine(linePoints[0], linePoints[1], getColor(multip * Vec4(1.0f, 0.0f, 0.0f, 1.0f)));
        lineRenderSystem.addLine(linePoints[0], linePoints[2], getColor(multip * Vec4(0.0f, 1.0f, 0.0f, 1.0f)));
        lineRenderSystem.addLine(linePoints[0], linePoints[4], getColor(multip * Vec4(0.0f, 0.0f, 1.0f, 1.0f)));

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

    lineRenderSystem.prepareToRender();

    if(selectedEntityIndex < entityIndices.size())
    {
        mouseHover = drawEntityImgui(scene.getEntity(selectedEntityIndex), selectedEntityIndex);
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