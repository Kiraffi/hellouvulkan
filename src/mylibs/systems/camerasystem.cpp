#include "camerasystem.h"

#include "app/glfw_keys.h"
#include "app/vulkan_app.h"
#include "container/podvector.h"
#include "core/camera.h"
#include "core/nullable.h"
#include "gpu/gpustructs.h"
#include "math/quaternion_inline_functions.h"
#include "math/vector3.h"
#include "math/vector3_inline_functions.h"
#include "app/inputapp.h"


struct CameraSystemData
{
    PodVector<Camera> m_cameras;
    Camera m_sunCamera;
    i32 m_currCameraIndex = 0;
    bool m_useSunCamera = false;
};

static Nullable<CameraSystemData> s_data;

Vector3 CameraSystem::getSunDirection()
{
    const auto& camera = s_data.get()->m_sunCamera;
    Vec3 sunDir[3];
    getDirectionsFromPitchYawRoll(camera.m_pitch, camera.m_yaw, 0.0f, sunDir[0], sunDir[1], sunDir[2]);

    return -sunDir[2];
}
Camera& CameraSystem::getCurrentCamera()
{
    return s_data.get()->m_cameras[s_data.get()->m_currCameraIndex];
}
void CameraSystem::fillGpuFrameBuffer(GpuFrameBuffer &buffer)
{
    const auto& app = VulkanApp::getWindowApp();

    auto& camera = getCurrentCamera();
    auto& sunCamera = s_data.get()->m_sunCamera;

    // bit ugly.......
    sunCamera.calculateOrtographicPosition(camera.m_position);
    if(app.windowWidth > 0 && app.windowHeight > 0)
        camera.updateCameraState(app.windowWidth, app.windowHeight);
    sunCamera.updateCameraState(50.0f, 50.0f);

    if (s_data.get()->m_useSunCamera)
    {
        buffer.mvp = sunCamera.m_worldToViewMat;
        buffer.inverseMvp = sunCamera.m_viewToWorldMat;
    }
    else
    {
        buffer.mvp = camera.m_worldToViewMat;
        buffer.inverseMvp = camera.m_viewToWorldMat;
    }
    buffer.sunMatrix = sunCamera.m_worldToViewMat;
    buffer.camPos = Vector4(camera.m_position, 0.0f);

}

bool CameraSystem::init()
{
    s_data.create();
    s_data.get()->m_cameras.pushBack(Camera{});
    s_data.get()->m_currCameraIndex = 0;
    s_data.get()->m_cameras[0].m_position = Vec3(0.0f, 4.0f, 5.0f);

    return true;
}

void CameraSystem::update()
{
    Camera& camera = getCurrentCamera();
    Camera& sunCamera = s_data.get()->m_sunCamera;
    float deltaTime = VulkanApp::getWindowApp().frameDt;

    //printf("deltatime: %f\n", deltaTime);
    float moveBooster = InputApp::isDown(GLFW_KEY_LEFT_SHIFT) ? 5.0f : 1.0f;
    float rotationBooster = InputApp::isDown(GLFW_KEY_LEFT_SHIFT) ? 2.0f : 1.0f;

    float moveSpeed = deltaTime * 2.0f * moveBooster;
    float rotationSpeed = deltaTime * 1.0f * rotationBooster;

    if (InputApp::isPressed(GLFW_KEY_RIGHT_BRACKET))
        s_data.get()->m_useSunCamera = !s_data.get()->m_useSunCamera;
    if (InputApp::isDown(GLFW_KEY_UP))
        sunCamera.m_pitch -= rotationSpeed;
    if (InputApp::isDown(GLFW_KEY_DOWN))
        sunCamera.m_pitch += rotationSpeed;
    if (InputApp::isDown(GLFW_KEY_LEFT))
        sunCamera.m_yaw += rotationSpeed;
    if (InputApp::isDown(GLFW_KEY_RIGHT))
        sunCamera.m_yaw -= rotationSpeed;
    while (sunCamera.m_pitch >= 2.0f * PI) sunCamera.m_pitch -= 2.0f * PI;
    while (sunCamera.m_pitch <= 0.0f) sunCamera.m_pitch += 2.0f * PI;
    while (sunCamera.m_yaw >= 2.0f * PI) sunCamera.m_yaw -= 2.0f * PI;
    while (sunCamera.m_yaw <= 0.0f) sunCamera.m_yaw += 2.0f * PI;




    if (InputApp::isDown(GLFW_KEY_I))
    {
        camera.m_pitch += rotationSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_K))
    {
        camera.m_pitch -= rotationSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_J))
    {
        camera.m_yaw += rotationSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_L))
    {
        camera.m_yaw -= rotationSpeed;
    }

    if (InputApp::isDown(GLFW_KEY_O))
    {
        camera.m_roll += rotationSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_P))
    {
        camera.m_roll -= rotationSpeed;
    }

    camera.m_pitch = Supa::clampf(camera.m_pitch, -0.499f * PI, 0.4999f * PI);
    camera.m_yaw = Supa::modf(camera.m_yaw, 2.0f * PI);
    camera.m_roll = Supa::modf(camera.m_roll, 2.0f * PI);

    Vec3 rightDir;
    Vec3 upDir;
    Vec3 forwardDir;

    // camera forward = -forward.
    getDirectionsFromPitchYawRoll(camera.m_pitch, camera.m_yaw, camera.m_roll, rightDir, upDir, forwardDir);


    if (InputApp::isDown(GLFW_KEY_W))
    {
        camera.m_position = camera.m_position - forwardDir * moveSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_S))
    {
        camera.m_position = camera.m_position + forwardDir * moveSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_A))
    {
        camera.m_position = camera.m_position - rightDir * moveSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_D))
    {
        camera.m_position = camera.m_position + rightDir * moveSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_Q))
    {
        camera.m_position = camera.m_position - upDir * moveSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_E))
    {
        camera.m_position = camera.m_position + upDir * moveSpeed;
    }

}

void CameraSystem::deinit()
{
    s_data.destroy();
}