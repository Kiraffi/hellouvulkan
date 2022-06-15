#pragma once

#include <math/vector3.h>

#include <math/matrix.h>

class FontRenderSystem;
struct Ray;

enum class CameraType
{
    ORTHO,
    PERSPECTIVE
};

struct Camera
{
    void updateCameraState(float width, float height);

    Matrix getPerspectiveProjection();
    Matrix getOrtographicProjection(float width, float height);
    Matrix getCameraMatrix();
    Matrix getCameraMatrix(const Vec3 &target);

    Vec2 renderCameraInfo(FontRenderSystem& fontSystem, Vec2 camInfoPosition, const Vec2& fontSize);
    void calculateOrtographicPosition(const Vec3& targetPos);

    Ray getRayFromNormalizedCoordinates(const Vec2 &normalizedCoordinates);
    Ray getRayFromScreenPixelCoordinates(const Vec2 &screenPixelCoordinates, const Vec2 &windowSize);
    Matrix worldToViewMat;
    Matrix viewToWorldMat;

    Vec3 position = Vec3(0.0f, 0.0f, -5.0f);
    
    float fovY = 100.0f;

    float width = 0.0f;
    float height = 0.0f;

    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;

    float zNear = 0.1f;
    float zFar = 200.0f;

    CameraType cameraType = CameraType::PERSPECTIVE;
};


