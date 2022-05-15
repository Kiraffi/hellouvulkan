#pragma once

#include <core/timer.h>
#include "myvulkan/vulkaninitparameters.h"
#include <render/font_render.h>

struct GLFWwindow;
struct Camera;

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
    virtual void update() = 0;
    virtual void resized() = 0;

    void run();
    void updateRenderFrameBuffer();

    void resizeWindow(int w, int h);
    void setVsyncEnabled(bool enable);
    void setClearColor(float r, float g, float b, float a);
    void setTitle(const char *str);

    void checkCameraKeypresses(float dt, Camera& camera);

    double getDeltaTime();
    double getTime();
    MouseState getMouseState();

    bool isPressed(int keyCode);
    bool isReleased(int keyCode);
    bool isDown(int keyCode);
    bool isUp(int keyCode);

public:

    GLFWwindow *window = nullptr;
    int windowWidth = 0;
    int windowHeight = 0;
    bool vSync = true;
    bool inited = false;
    bool needToResize = false;

    KeyState keyDowns[512] = {};
    uint32_t bufferedPresses[ 128 ] = {};
    uint32_t bufferedPressesCount = 0u;

protected:
    FontRenderSystem fontSystem;

    Timer timer;
    double dt = 0.0;
};
