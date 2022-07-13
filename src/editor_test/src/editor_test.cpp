
#include <components/transform_functions.h>

#include <container/podvector.h>

#include <core/camera.h>
#include <core/general.h>
#include <core/glfw_keys.h>
#include <core/json.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>
#include <core/writejson.h>

#include <container/stackstring.h>

#include <gui/editorsystem.h>

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
#include <render/lightingrendertargets.h>
#include <render/lightrendersystem.h>
#include <render/linerendersystem.h>
#include <render/meshrendersystem.h>
#include <render/meshrendertargets.h>
#include <render/myimguirenderer.h>
#include <render/tonemaprendersystem.h>
#include <render/viewport.h>

#include <scene/scene.h>

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
    EditorTest() : scene(meshRenderSystem), editorSystem(scene, lineRenderSystem, SCREEN_WIDTH, SCREEN_HEIGHT) { }
    virtual ~EditorTest() override;
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

    EditorSystem editorSystem;

    Vec2 fontSize{ 8.0f, 12.0f };
    float rotationAmount = 0.0f;

    bool showNormalMap = false;
    bool rotateOn = false;

    Viewport viewport;
    bool focusOnViewport = false;
};


////////////////////////
//
// DEINIT
//
////////////////////////

EditorTest::~EditorTest()
{
}



bool EditorTest::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;
    // TEMPORARY!
    //setWindowPosition(2000, 100);




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

    if(!editorSystem.init(window))
        return false;

    camera.position = Vec3(0.0f, 4.0f, 5.0f);
    sunCamera.pitch = toRadians(330.0f);
    sunCamera.yaw = toRadians(30.0f);

    return resized();
}

bool EditorTest::resized()
{
    uint32_t wholeWidth = vulk->swapchain.width;
    uint32_t wholeHeight = vulk->swapchain.height;
    viewport = editorSystem.getEditorWindowViewport();
    windowWidth = uint32_t(viewport.size.x);
    windowHeight = uint32_t(viewport.size.y);

    if (!meshRenderTargets.resizeMeshTargets(windowWidth, windowHeight))
        return false;
    if (!lightingRenderTargets.resizeLightingTargets(windowWidth, windowHeight))
        return false;

    meshRenderSystem.setRenderTargets(meshRenderTargets);
    lineRenderSystem.setRendertargets(meshRenderTargets.albedoImage, meshRenderTargets.depthImage);

    fontSystem.setRenderTarget(meshRenderTargets.albedoImage);
    convertFromS16.updateSourceImages(meshRenderTargets);

    lightRenderSystem.updateReadTargets(meshRenderTargets, lightingRenderTargets);
    tonemapRenderSystem.updateReadTargets(lightingRenderTargets.lightingTargetImage, meshRenderTargets.albedoImage);



    editorSystem.resizeWindow(meshRenderTargets.albedoImage);
    return true;
}



void EditorTest::logicUpdate()
{
    VulkanApp::logicUpdate();
    lineRenderSystem.clear();
    meshRenderSystem.clear();

    MouseState mouseState = getMouseState();

    editorSystem.logicUpdate(*this);

    if(!editorSystem.guiHasFocus())
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


}

void EditorTest::renderUpdate()
{
    windowWidth = uint32_t(viewport.size.x);
    windowHeight = uint32_t(viewport.size.y);

    VulkanApp::renderUpdate();

    editorSystem.renderUpdateGui();
    viewport = editorSystem.getEditorWindowViewport();

    if(viewport.size.x != windowWidth || viewport.size.y != windowHeight)
        vulk->needToResize = needToResize = true;

    editorSystem.renderUpdateViewport();

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
}

void EditorTest::renderDraw()
{
    // Drawingg
    {
        meshRenderTargets.prepareTargetsForMeshRendering();
        meshRenderTargets.prepareTargetsForShadowRendering();
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
    {
        prepareToGraphicsSampleRead(meshRenderTargets.albedoImage);
        editorSystem.renderDraw();
    }
    Image &finalImage = editorSystem.getRenderTargetImage();
    present(finalImage);
}

int main(int argCount, char **argv)
{
    initMemory();
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
    }
    deinitMemory();
    return 0;
}