#pragma once

#include <core/mytypes.h>

struct GLFWwindow;

struct WindowApp
{
    double appRuntime = 0.0;

    i32 windowWidth = 0;
    i32 windowHeight = 0;
    i32 viewportWidth = 0;
    i32 viewportHeight = 0;

    float cpuFps = 0.0f;
    float gpuFps = 0.0f;
    float cpudt = 0.0f;
    float gpudt = 0.0f;

    float frameDt = 0.0f;

    bool inited = false;
    bool resized = false;
};

class VulkanApp
{
public:
    static bool initApp(const char *windowStr, i32 screenWidth, i32 screenHeight);
    static void deinitApp();

    static const WindowApp& getWindowApp();
    static const GLFWwindow* getWindow();
    static GLFWwindow* getWindowRef();

    static bool updateApp();
    static void frameEnd();

    static void setTitle(const char *str);
    static void setWindowPosition(u32 x, u32 y);
};
