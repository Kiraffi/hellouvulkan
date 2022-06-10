#pragma once

#include <math/vector3.h>

class FontRenderSystem;
class Matrix;
struct Camera
{
    Matrix perspectiveProjection();
    Matrix ortographicProjection(float width, float height);
    Matrix getCameraMatrix();
    Matrix getCameraMatrix(const Vec3 &target);

    Vec2 renderCameraInfo(FontRenderSystem& fontSystem, Vec2 camInfoPosition, const Vec2& fontSize);
    void calculateOrtographicPosition(const Vec3& targetPos);

    Vec3 position = Vec3(0.0f, 0.0f, -5.0f);
    float aspectRatioWByH = 1.667f;
    float fovY = 100.0f;

    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;

    float zNear = 0.1f;
    float zFar = 200.0f;
};


