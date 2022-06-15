#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <components/transform_functions.h>

#include <core/camera.h>
#include <core/general.h>
#include <core/json.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>
#include <core/writejson.h>

#include <container/arraysliceview.h>
#include <container/stackstring.h>

#include <gui/componentviews.h>

#include <math/general_math.h>
#include <math/hitpoint.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion_inline_functions.h>
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

    return -sundir[2];
}

class EditorTest : public VulkanApp
{
public:
    EditorTest() : scene(meshRenderSystem) { }
    virtual ~EditorTest() override;
    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

public:
    bool drawSaveDialog();

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

    bool showSaveDialog = false;

    SmallStackString levelName = "assets/levels/testmap2.json";

    Vec3 lineFrom;
    Vec3 lineTo;
};


////////////////////////
//
// DEINIT
//
////////////////////////

EditorTest::~EditorTest()
{
}

bool EditorTest::drawSaveDialog()
{
    ImGui::OpenPopup("Save Dialog");

    bool saved = false;
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if(ImGui::BeginPopupModal("Save Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::Text("Save scene: \"%s\"", scene.getSceneName().getStr());
       
        char arr[30];
        memcpy(arr, levelName.getStr(), levelName.length() + 1);
        ImGui::InputText("Filename", arr, 30);
        levelName = arr;

        ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
        if(ImGui::Button("Save", button_size))
        {
            showSaveDialog = false;
            saved = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel", button_size))
        {
            showSaveDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    return saved;
}


bool EditorTest::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
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

    if(!scene.readLevel(levelName.getStr()))
        return false;

    camera.position = Vec3(0.0f, 4.0f, 5.0f);


    sunCamera.pitch = toRadians(330.0f);
    sunCamera.yaw = toRadians(30.0f);

    return resized();
}


bool EditorTest::resized()
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



void EditorTest::logicUpdate()
{
    VulkanApp::logicUpdate();
    lineRenderSystem.clear();
    meshRenderSystem.clear();

    MouseState mouseState = getMouseState();

    if(isPressed(GLFW_KEY_S) && (isDown(GLFW_KEY_LEFT_CONTROL) || isDown(GLFW_KEY_RIGHT_CONTROL)))
        showSaveDialog = true;

    if(!showSaveDialog)
    {
        checkCameraKeypresses(dt, camera);

        if(isPressed(GLFW_KEY_KP_ADD))
        {
            for(auto &entity : scene.getEntities())
            {
                ++entity.animationIndex;
            }
        }
        if(isPressed(GLFW_KEY_KP_SUBTRACT))
        {
            for(auto &entity : scene.getEntities())
            {
                if(entity.animationIndex > 0)
                    --entity.animationIndex;
            }
        }


        if(isPressed(GLFW_KEY_SPACE))
            showNormalMap = !showNormalMap;

        if(isPressed(GLFW_KEY_Z))
            rotateOn = !rotateOn;

        if(isDown(GLFW_KEY_UP))
            sunCamera.pitch -= dt * 1.0f;
        if(isDown(GLFW_KEY_DOWN))
            sunCamera.pitch += dt * 1.0f;
        if(isDown(GLFW_KEY_LEFT))
            sunCamera.yaw += dt * 1.0f;
        if(isDown(GLFW_KEY_RIGHT))
            sunCamera.yaw -= dt * 1.0f;

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
                lineFrom = ray.pos;
                lineTo = hitpoint.point;
            }
        }
    }

    Vec3 sundir = getSunDirection(sunCamera);
    while (sunCamera.pitch >= 2.0f * PI) sunCamera.pitch -= 2.0f * PI;
    while (sunCamera.pitch <= 0.0f) sunCamera.pitch += 2.0f * PI;
    while (sunCamera.yaw >= 2.0f * PI) sunCamera.yaw -= 2.0f * PI;
    while (sunCamera.yaw <= 0.0f) sunCamera.yaw += 2.0f * PI;

    Vec2 renderPos = camera.renderCameraInfo(fontSystem, Vec2(10.0f, 20.0f), fontSize);
    char tmpStr[1024];
    snprintf(tmpStr, 1024, "Show normal mode: %s, rotation enabled: %s, rotaion amount: %.2f, use sun camera: %s",
        showNormalMap ? "on" : "off", rotateOn ? "on" : "off", toDegrees(rotationAmount), useSunCamera ? "on" : "off");
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 0.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "SunPitch: %.3f, SunYaw: %.3f, Sundir: %.3f, %.3f, %.3f",
        toDegrees(sunCamera.pitch), toDegrees(sunCamera.yaw), sundir.x, sundir.y, sundir.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 1.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Sun pos: %.3f, %.3f, %.3f", sunCamera.position.x, sunCamera.position.y, sunCamera.position.z);
    fontSystem.addText(tmpStr, renderPos + Vec2(0.0f, fontSize.y * 2.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    lineRenderSystem.addLine(lineFrom, lineTo, getColor(0.0f, 1.0f, 0.0f, 1.0f));
}

void EditorTest::renderUpdate()
{
    VulkanApp::renderUpdate();
    imgui.renderBegin();
    
    if(ImGui::BeginMainMenuBar())
    {
        if(ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Open", "Ctrl+O")) {}            
            if(ImGui::MenuItem("Save", "Ctrl+S")) { showSaveDialog = true; }
            if(ImGui::MenuItem("Save As..")) {}

            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Edit"))
        {
            if(ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if(ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if(ImGui::MenuItem("Cut", "CTRL+X")) {}
            if(ImGui::MenuItem("Copy", "CTRL+C")) {}
            if(ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if(showSaveDialog && drawSaveDialog())
        scene.writeLevel(levelName.getStr());

    uint32_t grayColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.75f, 0.75f, 0.75f, 1.0f));
    uint32_t selectedColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.75f, 0.75f, 0.75f, 1.0f));


    uint32_t unSelectedRedColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    uint32_t unSelectedGreenColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    uint32_t unSelectedBlueColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.0f, 0.0f, 1.0f, 1.0f));


    uint32_t selectedRedColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    uint32_t selectedGreenColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    uint32_t selectedBlueColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.0f, 0.0f, 1.0f, 1.0f));


    for (const auto &entity : scene.getEntities())
    {
        Vec4 linePoints4[8];
        Vec3 linePoints[8];
        Mat3x4 m = getModelMatrix(entity.transform);
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
        mouseHover = drawEntity(scene.getEntity(selectedEntityIndex));
    }
    else
    {
        mouseHover = false;
    }

}

void EditorTest::renderDraw()
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
    EditorTest app;
    if (app.init("Editor", SCREEN_WIDTH, SCREEN_HEIGHT,
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