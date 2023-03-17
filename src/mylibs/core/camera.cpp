
#include "camera.h"

#include <app/glfw_keys.h>
#include <app/inputapp.h>
#include <app/vulkan_app.h>

#include <core/general.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/matrix_inline_functions.h>
#include <math/quaternion_inline_functions.h>
#include <math/ray.h>

#include <render/fontrendersystem.h>

#include <math.h>

// Rethink this... camera manager?
void Camera::checkCameraKeypresses()
{
    float deltaTime = VulkanApp::getWindowApp().frameDt;
    //printf("deltatime: %f\n", deltaTime);
    float moveBooster = InputApp::isDown(GLFW_KEY_LEFT_SHIFT) ? 5.0f : 1.0f;
    float rotationBooster = InputApp::isDown(GLFW_KEY_LEFT_SHIFT) ? 2.0f : 1.0f;

    float moveSpeed = deltaTime * 2.0f * moveBooster;
    float rotationSpeed = deltaTime * 1.0f * rotationBooster;

    if (InputApp::isDown(GLFW_KEY_I))
    {
        m_pitch += rotationSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_K))
    {
        m_pitch -= rotationSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_J))
    {
        m_yaw += rotationSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_L))
    {
        m_yaw -= rotationSpeed;
    }

    if (InputApp::isDown(GLFW_KEY_O))
    {
        m_roll += rotationSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_P))
    {
        m_roll -= rotationSpeed;
    }

    m_pitch = Supa::clampf(m_pitch, -0.499f * PI, 0.4999f * PI);
    m_yaw = Supa::modf(m_yaw, 2.0f * PI);
    m_roll = Supa::modf(m_roll, 2.0f * PI);

    Vec3 rightDir;
    Vec3 upDir;
    Vec3 forwardDir;

    // camera forward = -forward.
    getDirectionsFromPitchYawRoll(m_pitch, m_yaw, m_roll, rightDir, upDir, forwardDir);


    if (InputApp::isDown(GLFW_KEY_W))
    {
        m_position = m_position - forwardDir * moveSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_S))
    {
        m_position = m_position + forwardDir * moveSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_A))
    {
        m_position = m_position - rightDir * moveSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_D))
    {
        m_position = m_position + rightDir * moveSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_Q))
    {
        m_position = m_position - upDir * moveSpeed;
    }
    if (InputApp::isDown(GLFW_KEY_E))
    {
        m_position = m_position + upDir * moveSpeed;
    }
}


void Camera::updateCameraState(float width, float height)
{
    ASSERT(width > 0.0f && height > 0.0f);
    m_width = width;
    m_height = height;
    if(m_cameraType == CameraType::PERSPECTIVE)
    {
        m_worldToViewMat = getPerspectiveProjection() * getCameraMatrix();
    }
    else
    {
        m_worldToViewMat = getOrtographicProjection(width, height) * getCameraMatrix();
    }
    m_viewToWorldMat = inverse(m_worldToViewMat);
}

Matrix Camera::getPerspectiveProjection() const
{
    float f = 1.0f / tanf(toRadians(m_fovY * 0.5f));

    float s1 = -m_zFar / (m_zFar - m_zNear);
    float s2 = m_zNear * s1;
    float aspectRatioWByH = m_width / m_height;
    return Matrix(
        f / aspectRatioWByH, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, s1, s2,
        0.0f, 0.0f, -1.0f, 0.0f);
}

Matrix Camera::getOrtographicProjection(float width, float height) const
{
    float divider = m_zFar - m_zNear; //Near - zFar;
    float s1 = -1.0f / (divider);
    float s2 = -(m_zNear) / (divider);

    float x = 2.0f / width;
    float y = 2.0f / height;

    return Matrix(
        x, 0.0f, 0.0f, 0.0f,
        0.0f, y, 0.0f, 0.0f,
        0.0f, 0.0f, s1, s2,
        0.0f, 0.0f, 0.0f, 1.0f);
}

void Camera::calculateOrtographicPosition(const Vec3 &targetPos)
{
    Vector3 rightDir;
    Vector3 upDir;
    Vector3 forwardDir;
    // Invert the camera rotations
    getDirectionsFromPitchYawRoll(m_pitch, m_yaw, m_roll, rightDir, upDir, forwardDir);

    m_position = targetPos + forwardDir * 100.0f;
}

Matrix Camera::getCameraMatrix() const
{
    Vector3 rightDir;
    Vector3 upDir;
    Vector3 forwardDir;
    // Invert the camera rotations
    getDirectionsFromPitchYawRoll(m_pitch, m_yaw, m_roll, rightDir, upDir, forwardDir);
    return createMatrixFromLookAt(m_position, m_position - forwardDir, upDir);
}

void Camera::lookAt(const Vec3 &targetPos)
{
    Vec3 cameraDir = normalize(targetPos - m_position);
    m_roll = 0.0f;

    m_pitch = Supa::asinf(cameraDir.y);
    m_yaw = Supa::atan2f(-cameraDir.x, -cameraDir.z);
}

Matrix Camera::getCameraMatrix(const Vector3 &target) const
{
    return createMatrixFromLookAt(m_position, target, Vec3(0.0f, 1.0f, 0.0));
}

Vec2 Camera::renderCameraInfo(Vec2 camInfoPosition, const Vec2& fontSize) const
{
    char tmpStr[1024];

    snprintf(tmpStr, 1024, "Camera position: (%.2f, %.2f, %.2f), pitch: %.2f, yaw:%.2f, roll:%.2f",
        m_position.x, m_position.y, m_position.z, m_pitch, m_yaw, m_roll);
    FontRenderSystem::addText(tmpStr, camInfoPosition, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

    Vec3 cameraRightdDir;
    Vec3 cameraUpDir;
    Vec3 cameraForwardDir;
    getDirectionsFromPitchYawRoll(-m_pitch, -m_yaw, -m_roll, cameraRightdDir, cameraUpDir, cameraForwardDir);

    camInfoPosition.y += fontSize.y + 2.0f;

    snprintf(tmpStr, 1024, "Camera forward (-z): (%.3f, %.3f, %.3f)", -cameraForwardDir.x, -cameraForwardDir.y, -cameraForwardDir.z);
    FontRenderSystem::addText(tmpStr, camInfoPosition + Vec2(0.0f, fontSize.y) * 0.0f, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Camera up: (%.3f, %.3f, %.3f)", cameraUpDir.x, cameraUpDir.y, cameraUpDir.z);
    FontRenderSystem::addText(tmpStr, camInfoPosition + Vec2(0.0f, fontSize.y) * 1.0f, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Camera right: (%.3f, %.3f, %.3f)", cameraRightdDir.x, cameraRightdDir.y, cameraRightdDir.z);
    FontRenderSystem::addText(tmpStr, camInfoPosition + Vec2(0.0f, fontSize.y) * 2.0f, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    return camInfoPosition + Vec2(0.0f, fontSize.y) * 3.0f;
}


Ray Camera::getRayFromNormalizedCoordinates(const Vec2 &normalizedCoordinates) const
{
    Ray result{ Uninit };

    // Calculate the click as ndc. Half pixel offset as rendering.
    Vec2 coord = normalizedCoordinates;
    coord = coord * 2.0f - 1.0f;
    coord.y = -coord.y;

    Vec4 rayDir4 = mul(m_viewToWorldMat, Vec4(coord.x, coord.y, 1.0f, 1.0f));
    rayDir4 = rayDir4 / rayDir4.w;
    Vec3 rayDir(rayDir4.x, rayDir4.y, rayDir4.z);

    Vec3 fromPos{ Uninit };
    if(m_cameraType == CameraType::PERSPECTIVE)
    {
        fromPos = m_position;
    }
    else
    {
        Vec4 fromDir4 = mul(m_viewToWorldMat, Vec4(coord.x, coord.y, 0.0f, 1.0f));
        fromDir4 = fromDir4 / fromDir4.w;
        fromPos = Vec3(fromDir4.x, fromDir4.y, fromDir4.z);
    }

    rayDir = normalize(rayDir - fromPos);
    return Ray(fromPos, rayDir);
}

Ray Camera::getRayFromScreenPixelCoordinates(const Vec2 &screenPixelCoordinates, const Vec2 &windowSize) const
{
    // Calculate the click as ndc. Half pixel offset as rendering.
    Vec2 coord = (screenPixelCoordinates + 0.5f) / windowSize;
    return getRayFromNormalizedCoordinates(coord);
}

Ray Camera::getRayFromScreenPixelCoordinates(const Vec2 &screenPixelCoordinates) const
{
    const auto& app = VulkanApp::getWindowApp();
    Vec2 windowSize = Vec2(app.windowWidth, app.windowHeight);

    // Calculate the click as ndc. Half pixel offset as rendering.
    Vec2 coord = (screenPixelCoordinates + 0.5f) / windowSize;
    return getRayFromNormalizedCoordinates(coord);
}
