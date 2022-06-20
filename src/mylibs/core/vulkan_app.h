#pragma once

#include <core/camera.h>
#include <core/timer.h>
#include <myvulkan/vulkaninitparameters.h>
#include <render/font_render.h>

struct GLFWwindow;
struct Vector2;

struct MouseState
{
    int x;
    int y;
    bool leftButtonDown;
    bool rightButtonDown;
    bool middleButtonDown;
};

struct KeyState
{
    bool isDown = false;
    uint32_t pressCount = 0u;
};

class VulkanApp
{
public:
    VulkanApp() {}
    virtual ~VulkanApp();

    virtual bool init(const char *windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters &initParameters) = 0;
    virtual void logicUpdate() = 0;
    virtual void renderUpdate() = 0;
    virtual void renderDraw() = 0;
    virtual bool resized() = 0;

    void run();

    void resizeWindow(int w, int h);
    void setVsync(VSyncType vSyncType);
    void setClearColor(float r, float g, float b, float a);
    void setTitle(const char *str);
    void setWindowPosition(uint32_t x, uint32_t y);
    const Camera &getActiveCamera() const;

    Vector2 getWindowSize() const;

    void checkCameraKeypresses(float dt, Camera& camera);

    double getDeltaTime() const;
    double getTime() const;
    MouseState getMouseState() const;

    bool isPressed(int keyCode) const;
    bool isReleased(int keyCode) const;
    bool isDown(int keyCode) const;
    bool isUp(int keyCode) const;

public:
    GLFWwindow *window = nullptr;

    KeyState keyDowns[512] = {};
    uint32_t bufferedPresses[128] = {};
    uint32_t bufferedPressesCount = 0u;

    double dt = 0.0;

    int windowWidth = 0;
    int windowHeight = 0;
    int viewportWidth = 0;
    int viewportHeight = 0;
    VSyncType vSync = VSyncType::FIFO_VSYNC;
    bool inited = false;
    bool needToResize = false;

    float cpuFps = 0.0f;
    float gpuFps = 0.0f;
    float cpudt = 0.0f;
    float gpudt = 0.0f;

protected:
    FontRenderSystem fontSystem;

    Timer timer;

    // rotation in angles. TODO think better place later on!!!
    // needs even shadow map texture size
    Camera camera;
    Camera sunCamera;

    bool useSunCamera = false;
};
