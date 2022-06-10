
#include "camera.h"

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <render/font_render.h>

#include <math.h>
#include <stdio.h>

Matrix Camera::perspectiveProjection()
{
    float f = 1.0f / tanf(toRadians(fovY * 0.5f));

    float s1 = -zFar / (zFar - zNear);
    float s2 = zNear * s1;

    return Matrix(
        f / aspectRatioWByH, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, s1, s2,
        0.0f, 0.0f, -1.0f, 0.0f);
}



Matrix Camera::ortographicProjection(float width, float height)
{
    float divider = zFar - zNear; //Near - zFar;
    float s1 = -1.0f / (divider);
    float s2 = -(zNear) / (divider);

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
    getDirectionsFromPitchYawRoll(pitch, yaw, roll, rightDir, upDir, forwardDir);

    position = targetPos + forwardDir * 100.0f;
}

Matrix Camera::getCameraMatrix()
{
    Vector3 rightDir;
    Vector3 upDir;
    Vector3 forwardDir;
    // Invert the camera rotations
    getDirectionsFromPitchYawRoll(pitch, yaw, roll, rightDir, upDir, forwardDir);
    return createMatrixFromLookAt(position, position - forwardDir, upDir);
}

Matrix Camera::getCameraMatrix(const Vector3 &target)
{
    return createMatrixFromLookAt(position, target, Vec3(0.0f, 1.0f, 0.0));
}

Vec2 Camera::renderCameraInfo(FontRenderSystem& fontSystem, Vec2 camInfoPosition, const Vec2& fontSize)
{
    char tmpStr[1024];

    snprintf(tmpStr, 1024, "Camera position: (%.2f, %.2f, %.2f), pitch: %.2f, yaw:%.2f, roll:%.2f", position.x, position.y, position.z, pitch, yaw, roll);
    fontSystem.addText(tmpStr, camInfoPosition, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));


    Vec3 cameraRightdDir;
    Vec3 cameraUpDir;
    Vec3 cameraForwardDir;
    getDirectionsFromPitchYawRoll(-pitch, -yaw, -roll, cameraRightdDir, cameraUpDir, cameraForwardDir);

    camInfoPosition.y += fontSize.y + 2.0f;

    snprintf(tmpStr, 1024, "Camera forward (-z): (%.3f, %.3f, %.3f)", -cameraForwardDir.x, -cameraForwardDir.y, -cameraForwardDir.z);
    fontSystem.addText(tmpStr, camInfoPosition + Vec2(0.0f, fontSize.y) * 0.0f, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Camera up: (%.3f, %.3f, %.3f)", cameraUpDir.x, cameraUpDir.y, cameraUpDir.z);
    fontSystem.addText(tmpStr, camInfoPosition + Vec2(0.0f, fontSize.y) * 1.0f, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    snprintf(tmpStr, 1024, "Camera right: (%.3f, %.3f, %.3f)", cameraRightdDir.x, cameraRightdDir.y, cameraRightdDir.z);
    fontSystem.addText(tmpStr, camInfoPosition + Vec2(0.0f, fontSize.y) * 2.0f, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    return camInfoPosition + Vec2(0.0f, fontSize.y) * 3.0f;
}