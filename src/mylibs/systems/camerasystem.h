#pragma once

struct Camera;
struct GpuFrameBuffer;
struct Vector3;

class CameraSystem
{
public:
    static bool init();
    static void deinit();
    static void update();

    static Vector3 getSunDirection();
    static void fillGpuFrameBuffer(GpuFrameBuffer &buffer);
    static Camera& getCurrentCamera();
    static Camera& getSunCamera();
    static Camera& getCurrentIndexedCamera();

    static bool useSunCamera();
};

