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

    Matrix getPerspectiveProjection() const;
    Matrix getOrtographicProjection(float width, float height) const;
    Matrix getCameraMatrix() const;
    Matrix getCameraMatrix(const Vec3 &target) const;

    Vec2 renderCameraInfo(Vec2 camInfoPosition, const Vec2& fontSize) const;
    void calculateOrtographicPosition(const Vec3& targetPos);

    void lookAt(const Vec3 &targetPos);

    Ray getRayFromNormalizedCoordinates(const Vec2 &normalizedCoordinates) const;
    Ray getRayFromScreenPixelCoordinates(const Vec2 &screenPixelCoordinates, const Vec2 &windowSize) const;

    // Rethink this... camera manager?
    void checkCameraKeypresses();

    Matrix m_worldToViewMat;
    Matrix m_viewToWorldMat;

    Vec3 m_position = Vec3(0.0f, 0.0f, -5.0f);

    float m_fovY = 100.0f;

    float m_width = 0.0f;
    float m_height = 0.0f;

    float m_pitch = 0.0f;
    float m_yaw = 0.0f;
    float m_roll = 0.0f;

    float m_zNear = 0.1f;
    float m_zFar = 200.0f;

    CameraType m_cameraType = CameraType::PERSPECTIVE;
};


