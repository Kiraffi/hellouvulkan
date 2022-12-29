#include <app/glfw_keys.h>
#include <app/inputapp.h>
#include <app/vulkan_app.h>

#include <components/transform_functions.h>

#include <container/podvector.h>

#include <core/camera.h>
#include <core/general.h>
#include <core/json.h>
#include <core/timer.h>
#include <core/mytypes.h>

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
#include <myvulkan/vulkaninitparameters.h>
#include <myvulkan/vulkanresources.h>

#include <render/convertrendertarget.h>
#include <render/fontrendersystem.h>
#include <render/lightrendersystem.h>
#include <render/linerendersystem.h>
#include <render/meshrendersystem.h>
#include <render/tonemaprendersystem.h>

#include <render/lightingrendertargets.h>
#include <render/meshrendertargets.h>

#include <resources/globalresources.h>

#include <scene/scene.h>

static constexpr i32 c_ScreenWidth = 1024;
static constexpr i32 c_ScreenHeight = 768;

static constexpr i32 c_ShadowWidth = 2048;
static constexpr i32 c_ShadowHeight = 2048;

static Vec3 getSunDirection(const Camera &camera)
{
    Vec3 sundir[3];
    getDirectionsFromPitchYawRoll(camera.m_pitch, camera.m_yaw, 0.0f, sundir[0], sundir[1], sundir[2]);

    return -sundir[2];
}

struct VulkanDrawStuffData
{
    Scene m_scene;
    LightRenderSystem m_lightRenderSystem;
    TonemapRenderSystem m_tonemapRenderSystem;
    LineRenderSystem m_lineRenderSystem;

    LightingRenderTargets m_lightingRenderTargets;
    MeshRenderTargets m_meshRenderTargets;

    ConvertRenderTarget m_convertFromS16{ VK_FORMAT_R16G16B16A16_SNORM };
    Vec2 m_fontSize{ 8.0f, 12.0f };

    Camera m_camera;
    Camera m_sunCamera;

    u32 m_selectedEntityIndex = ~0u;
    float m_rotationAmount = 0.0f;

    bool m_showNormalMap = false;
    bool m_rotateOn = false;
    bool m_useSunCamera = false;

    Vec3 m_lineFrom;
    Vec3 m_lineTo;
};
VulkanDrawStuffData* s_data = nullptr;

static bool sResize();
static void sResized(int width, int height);
static void sDeinit();
static bool sInit(const char *windowStr, i32 screenWidth, i32 screenHeight);

////////////////////////
//
// DEINIT
//
////////////////////////

static void sDeinit()
{
    FontRenderSystem::deinit();
    MeshRenderSystem::deinit();
    if(s_data)
    {
        delete s_data;
        s_data = nullptr;
    }
    MyVulkan::deinit();
    VulkanApp::deinitApp();
}



static bool sInit(const char* windowStr, i32 screenWidth, i32 screenHeight)
{
    s_data = new VulkanDrawStuffData();
    auto &vulkanInitParams = VulkanInitializationParameters::getRef();
    vulkanInitParams.useIntegratedGpu = true;

    if(!VulkanApp::initApp(windowStr, screenWidth, screenHeight)
        || !InputApp::init()
        || !MyVulkan::init()
        || !FontRenderSystem::init("assets/font/new_font.dat"))
    {
        return false;
    }

    // TEMPORARY!
    //glfwSetWindowPos(window, 2000, 100);

    if (!s_data->m_convertFromS16.init(ShaderType::ConvertFromRGBAS16))
        return false;

    if (!MeshRenderSystem::init())
        return false;

    if (!s_data->m_scene.init())
        return false;

    if (!s_data->m_lightRenderSystem.init())
        return false;

    if (!s_data->m_tonemapRenderSystem.init())
        return false;

    if (!s_data->m_meshRenderTargets.resizeShadowTarget(c_ShadowWidth, c_ShadowHeight))
        return false;

    if (!s_data->m_lineRenderSystem.init())
        return false;

    s_data->m_camera.m_position = Vec3(0.0f, 4.0f, 5.0f);

    s_data->m_scene.addGameEntity({ .transform = {.pos = {0.0f, -0.1f, 0.0f }, .scale = { 10.0f, 1.0f, 10.0f } }, .entityType = EntityType::FLOOR });

    s_data->m_scene.addGameEntity({ .transform = {.pos = {3.0f, 1.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING});
    s_data->m_scene.addGameEntity({ .transform = {.pos = {-3.0f, 1.0f, 0.0f } }, .entityType = EntityType::WOBBLY_THING });

    s_data->m_scene.addGameEntity({ .transform = {.pos = {0.0f, 0.1f, -15.0f } }, .entityType = EntityType::CHARACTER });

    s_data->m_scene.addGameEntity({ .transform = {.pos = {-3.0f, 0.1f, -20.0f } }, .entityType = EntityType::LOW_POLY_CHAR });
    s_data->m_scene.addGameEntity({ .transform = {.pos = {3.0f, 0.1f, -20.0f } }, .entityType = EntityType::LOW_POLY_CHAR });

    for(i32 y = -10; y < 10; ++y)
    {
        for(i32 x = -150; x < 150; ++x)
        {
            Vec3 pos(x, 0.0f, -50.0f + y);
            pos.x += 0.5f;
            pos.y += 0.5f;

            s_data->m_scene.addGameEntity({ .transform = { .pos = pos }, .entityType = EntityType::LOW_POLY_CHAR });
        }
    }


    for (float f = -2.5f; f <= 2.5f; f += 1.0f)
    {
        s_data->m_scene.addGameEntity({.transform = {.pos = {f * 5.0f, 1.0f, -5.0f}, .scale = {0.1f, 0.1f, 0.1f } }, .entityType = EntityType::ARROW });
        s_data->m_scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 10.0f}, }, .entityType = EntityType::TREE });
        s_data->m_scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 15.0f}, }, .entityType = EntityType::TREE_SMOOTH });
        s_data->m_scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 20.0f}, }, .entityType = EntityType::BLOB });
        s_data->m_scene.addGameEntity({ .transform = {.pos = {f * 5.0f, 0.0f, 25.0f}, }, .entityType = EntityType::BLOB_FLAT });
    }
    s_data->m_scene.addGameEntity({ .transform = {.pos = {0.0f, 1.0f, 2.0f } }, .entityType = EntityType::TEST_THING });



    s_data->m_sunCamera.m_pitch = toRadians(330.0f);
    s_data->m_sunCamera.m_yaw = toRadians(30.0f);

    MyVulkan::setVulkanFrameResizedCBFunc(sResized);

    return sResize();
}

static void sResized(int w, int h)
{
    sResize();
}

static bool sResize()
{
    if (!s_data->m_meshRenderTargets.resizeMeshTargets())
        return false;
    if (!s_data->m_lightingRenderTargets.resizeLightingTargets())
        return false;

    MeshRenderSystem::setRenderTargets(s_data->m_meshRenderTargets);
    s_data->m_lineRenderSystem.setRendertargets(
        s_data->m_meshRenderTargets.albedoImage, s_data->m_meshRenderTargets.depthImage);
    FontRenderSystem::setRenderTarget(s_data->m_meshRenderTargets.albedoImage);
    s_data->m_convertFromS16.updateSourceImages(s_data->m_meshRenderTargets);

    s_data->m_lightRenderSystem.updateReadTargets(s_data->m_meshRenderTargets, s_data->m_lightingRenderTargets);
    s_data->m_tonemapRenderSystem.updateReadTargets(
        s_data->m_lightingRenderTargets.lightingTargetImage, s_data->m_meshRenderTargets.albedoImage);

    return true;
}



static void sHandleInput()
{
    static u32 counter = 0;
    if(counter++ >= 100)
    {
        //printf("Matrix time: %f\n", float(getMatrixTime() / counter));
        //printf("Bytebuffer time: %f\n", float(getByteBufferTimer() / counter));
        counter = 0;
    }
    s_data->m_lineRenderSystem.clear();

    MouseState mouseState = InputApp::getMouseState();
    float dt = VulkanApp::getWindowApp().frameDt;

    s_data->m_camera.checkCameraKeypresses();

    if (InputApp::isPressed(GLFW_KEY_RIGHT_BRACKET))
    {
        s_data->m_useSunCamera = !s_data->m_useSunCamera;
    }

    if (InputApp::isPressed(GLFW_KEY_KP_ADD))
    {
        for (auto &entity : s_data->m_scene.getEntities())
        {
            ++entity.animationIndex;
        }
    }
    if (InputApp::isPressed(GLFW_KEY_KP_SUBTRACT))
    {
        for(auto &entity : s_data->m_scene.getEntities())
        {
            if(entity.animationIndex > 0)
                --entity.animationIndex;
        }
    }

    if (InputApp::isPressed(GLFW_KEY_SPACE))
        s_data->m_showNormalMap = !s_data->m_showNormalMap;

    if (InputApp::isPressed(GLFW_KEY_Z))
        s_data->m_rotateOn = !s_data->m_rotateOn;

    Camera& sunCamera = s_data->m_sunCamera;

    if (InputApp::isDown(GLFW_KEY_UP))
        sunCamera.m_pitch -= dt * 1.0f;
    if (InputApp::isDown(GLFW_KEY_DOWN))
        sunCamera.m_pitch += dt * 1.0f;
    if (InputApp::isDown(GLFW_KEY_LEFT))
        sunCamera.m_yaw += dt * 1.0f;
    if (InputApp::isDown(GLFW_KEY_RIGHT))
        sunCamera.m_yaw -= dt * 1.0f;

    Vec3 sundir = getSunDirection(sunCamera);
    while (sunCamera.m_pitch >= 2.0f * PI) sunCamera.m_pitch -= 2.0f * PI;
    while (sunCamera.m_pitch <= 0.0f) sunCamera.m_pitch += 2.0f * PI;
    while (sunCamera.m_yaw >= 2.0f * PI) sunCamera.m_yaw -= 2.0f * PI;
    while (sunCamera.m_yaw <= 0.0f) sunCamera.m_yaw += 2.0f * PI;

    const Vec2& fontSize = s_data->m_fontSize;

    Vec2 renderPos = s_data->m_camera.renderCameraInfo(Vec2(10.0f, 10.0f), fontSize);
    char tmpStr[1024];
    snprintf(tmpStr, 1024, "Show normal mode: %s, rotation enabled: %s, rotaion amount: %.2f, use sun camera: %s",
        s_data->m_showNormalMap
            ? "on" : "off", s_data->m_rotateOn ? "on"
            : "off", toDegrees(s_data->m_rotationAmount), s_data->m_useSunCamera
                ? "on" : "off");
    FontRenderSystem::addText(tmpStr,
        renderPos + Vec2(0.0f, fontSize.y * 0.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "SunPitch: %.3f, SunYaw: %.3f, Sundir: %.3f, %.3f, %.3f",
        toDegrees(sunCamera.m_pitch), toDegrees(sunCamera.m_yaw), sundir.x, sundir.y, sundir.z);
    FontRenderSystem::addText(tmpStr,
        renderPos + Vec2(0.0f, fontSize.y * 1.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Sun pos: %.3f, %.3f, %.3f",
        sunCamera.m_position.x, sunCamera.m_position.y, sunCamera.m_position.z);
    FontRenderSystem::addText(tmpStr,
        renderPos + Vec2(0.0f, fontSize.y * 2.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    MeshRenderSystem::clear();

    if(mouseState.leftButtonDown &&
        mouseState.x >= 0 && mouseState.y >= 0 &&
        mouseState.x < vulk->swapchain.width && mouseState.y < vulk->swapchain.height)
    {
        s_data->m_selectedEntityIndex = ~0u;

        Vec2 coord = Vec2(mouseState.x, mouseState.y);
        const auto& app = VulkanApp::getWindowApp();
        Vec2 windowSize = Vec2(app.windowWidth, app.windowHeight);
        Ray ray{ Uninit };
        if(s_data->m_useSunCamera)
            ray = s_data->m_sunCamera.getRayFromScreenPixelCoordinates(coord, windowSize);
        else
            ray = s_data->m_camera.getRayFromScreenPixelCoordinates(coord, windowSize);

        HitPoint hitpoint{ Uninit };
        s_data->m_selectedEntityIndex = s_data->m_scene.castRay(ray, hitpoint);
        if(s_data->m_selectedEntityIndex != ~0u)
        {
            s_data->m_lineTo = s_data->m_lineTo = hitpoint.point;
            s_data->m_lineFrom = ray.pos;
        }
    }

    s_data->m_lineRenderSystem.addLine(s_data->m_lineFrom, s_data->m_lineTo, getColor(0.0f, 1.0f, 0.0f, 1.0f));
}

static void sRenderUpdate()
{
    const auto& app = VulkanApp::getWindowApp();
    float dt = app.frameDt;

    vulk->queryPoolIndexCounts[vulk->frameIndex] = 0u;

    //beginSingleTimeCommands();
    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(vulk->commandBuffer, &beginInfo));

    vkCmdResetQueryPool(vulk->commandBuffer, vulk->queryPools[vulk->frameIndex], 0, QUERY_COUNT);

    MyVulkan::writeStamp();
    VulkanResources::update();

    struct FrameBuffer
    {
        Matrix camMat;
        Matrix viewProj;
        Matrix mvp;
        Matrix sunMatrix;

        Vector2 areaSize;
        Vector2 tmp1;

        Vector4 camPos;
        Vector4 tmp3;
        Vector4 tmp4;

        Matrix inverseMvp;
    };
    FrameBuffer frameBufferData;

    // bit ugly.......
    s_data->m_sunCamera.calculateOrtographicPosition(s_data->m_camera.m_position);
    if(app.windowWidth > 0 && app.windowHeight > 0)
        s_data->m_camera.updateCameraState(app.windowWidth, app.windowHeight);
    s_data->m_sunCamera.updateCameraState(50.0f, 50.0f);

    if (s_data->m_useSunCamera)
    {
        frameBufferData.mvp = s_data->m_sunCamera.m_worldToViewMat;
        frameBufferData.inverseMvp = s_data->m_sunCamera.m_viewToWorldMat;
    }
    else
    {
        frameBufferData.mvp = s_data->m_camera.m_worldToViewMat;
        frameBufferData.inverseMvp = s_data->m_camera.m_viewToWorldMat;
    }
    frameBufferData.sunMatrix = s_data->m_sunCamera.m_worldToViewMat;
    frameBufferData.camPos = Vector4(s_data->m_camera.m_position, 0.0f);

    frameBufferData.areaSize = Vec2(app.windowWidth, app.windowHeight);
    VulkanResources::addToCopylist(frameBufferData, vulk->renderFrameBufferHandle[vulk->frameIndex]);

    FontRenderSystem::update();

    //VulkanResources::addToCopylist(
    //    sliceFromPodVectorBytes(s_data->m_vertData), s_data->m_quadBuffer[vulk->frameIndex]);

    u32 grayColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.75f, 0.75f, 0.75f, 1.0f));
    u32 selectedColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.75f, 0.75f, 0.75f, 1.0f));

    u32 unSelectedRedColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    u32 unSelectedGreenColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    u32 unSelectedBlueColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.0f, 0.0f, 1.0f, 1.0f));

    u32 selectedRedColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    u32 selectedGreenColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    u32 selectedBlueColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.0f, 0.0f, 1.0f, 1.0f));

    for (auto &entity : s_data->m_scene.getEntities())
    {
        Vec4 linePoints4[8];
        Vec3 linePoints[8];

        Mat3x4 m = getModelMatrix(entity.transform);
        const auto &bounds = s_data->m_scene.getBounds(entity.index);
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

        for(u32 i = 0; i < 8; ++i)
            linePoints[i] = Vec3(linePoints4[i].x, linePoints4[i].y, linePoints4[i].z);

        Vec4 multip(0.5f, 0.5f, 0.5f, 1.0f);
        u32 drawColor = s_data->m_selectedEntityIndex == entity.index ? selectedColor : grayColor;
        s_data->m_lineRenderSystem.addLine(linePoints[1], linePoints[3], drawColor);
        s_data->m_lineRenderSystem.addLine(linePoints[2], linePoints[3], drawColor);
        s_data->m_lineRenderSystem.addLine(linePoints[1], linePoints[5], drawColor);
        s_data->m_lineRenderSystem.addLine(linePoints[2], linePoints[6], drawColor);
        s_data->m_lineRenderSystem.addLine(linePoints[3], linePoints[7], drawColor);
        s_data->m_lineRenderSystem.addLine(linePoints[4], linePoints[5], drawColor);
        s_data->m_lineRenderSystem.addLine(linePoints[4], linePoints[6], drawColor);
        s_data->m_lineRenderSystem.addLine(linePoints[5], linePoints[7], drawColor);
        s_data->m_lineRenderSystem.addLine(linePoints[6], linePoints[7], drawColor);

        u32 redColor = s_data->m_selectedEntityIndex == entity.index ? selectedRedColor : unSelectedRedColor;
        u32 greenColor = s_data->m_selectedEntityIndex == entity.index ? selectedGreenColor : unSelectedGreenColor;
        u32 blueColor = s_data->m_selectedEntityIndex == entity.index ? selectedBlueColor : unSelectedBlueColor;

        s_data->m_lineRenderSystem.addLine(linePoints[0], linePoints[1], redColor);
        s_data->m_lineRenderSystem.addLine(linePoints[0], linePoints[2], greenColor);
        s_data->m_lineRenderSystem.addLine(linePoints[0], linePoints[4], blueColor);

        if (entity.entityType == EntityType::NUM_OF_ENTITY_TYPES ||
            entity.entityType == EntityType::FLOOR)
            continue;

        entity.transform.rot = getQuaternionFromAxisAngle(Vec3(0.0f, 0.0f, 1.0f), s_data->m_rotationAmount);
    }
    if (s_data->m_rotateOn)
    {
        s_data->m_rotationAmount += 1.5f * dt;
        while (s_data->m_rotationAmount >= 2.0f * PI) s_data->m_rotationAmount -= 2.0f * PI;
        while (s_data->m_rotationAmount <= -2.0f * PI) s_data->m_rotationAmount += 2.0f * PI;
    }

    s_data->m_scene.update(dt);
    MeshRenderSystem::prepareToRender();

    Vec3 sundir = getSunDirection(s_data->m_sunCamera);
    s_data->m_lightRenderSystem.setSunDirection(sundir);

    s_data->m_lightRenderSystem.update();

    s_data->m_lineRenderSystem.prepareToRender();

}

static void sDraw()
{
    s_data->m_meshRenderTargets.prepareTargetsForMeshRendering();
    s_data->m_meshRenderTargets.prepareTargetsForShadowRendering();
    // Drawingg
    {
        MeshRenderSystem::render(s_data->m_meshRenderTargets);
        MeshRenderSystem::renderShadows(s_data->m_meshRenderTargets);
    }

    if (s_data->m_showNormalMap)
    {
        VulkanResources::prepareToComputeSampleRead(s_data->m_meshRenderTargets.normalMapImage);

        VulkanResources::prepareToComputeImageWrite(s_data->m_meshRenderTargets.albedoImage);
        Image& image = s_data->m_meshRenderTargets.albedoImage;
        s_data->m_convertFromS16.render(image.width, image.height);
    }
    else
    {
        {
            s_data->m_meshRenderTargets.prepareTargetsForLightingComputeSampling();
            s_data->m_lightingRenderTargets.prepareTargetsForLightingComputeWriting();

            Image& image = s_data->m_lightingRenderTargets.lightingTargetImage;
            s_data->m_lightRenderSystem.render(image.width, image.height);
        }

        {
            s_data->m_lightingRenderTargets.prepareForTonemapSampling();
            Image& image = s_data->m_meshRenderTargets.albedoImage;
            VulkanResources::prepareToComputeImageWrite(image);
            s_data->m_tonemapRenderSystem.render(image.width, image.height);
        }
    }

    VulkanResources::prepareToGraphicsSampleWrite(s_data->m_meshRenderTargets.albedoImage);
    {
        VulkanResources::prepareToGraphicsSampleWrite(s_data->m_meshRenderTargets.depthImage);
        s_data->m_lineRenderSystem.render(s_data->m_meshRenderTargets.albedoImage,
            s_data->m_meshRenderTargets.depthImage);
    }

    {
        FontRenderSystem::render();
    }


    MyVulkan::present(s_data->m_meshRenderTargets.albedoImage);
}


static void sRunApp()
{
    while(VulkanApp::updateApp())
    {
        sHandleInput();

        if (MyVulkan::frameStart())
        {
            //updateStats(*this);
            vulk->currentStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            sRenderUpdate();
            sDraw();
            //renderDraw();
            //printStats(*this);
        }

        defragMemory();
        VulkanApp::frameEnd();
    }
    VK_CHECK(vkDeviceWaitIdle(vulk->device));
}

i32 main(i32 argCount, char **argv)
{
    initMemory();
    initGlobalResources();
    if(sInit("Vulkan, draw models", c_ScreenWidth, c_ScreenHeight))
    {
        sRunApp();
    }
    sDeinit();
    deinitGlobalResources();
    deinitMemory();
    return 0;
}

/*
i32 main(i32 argCount, char **argv)
{
    initMemory();
    {
        VulkanDrawStuff app;
        if (app.init("Vulkan, draw models", SCREEN_WIDTH, SCREEN_HEIGHT))
        {
            app.run();
        }
    }
    deinitMemory();
    return 0;
}
*/