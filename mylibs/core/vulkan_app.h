#pragma once

#include "core/timer.h"


//#include "myvulkan/vulkandevice.h"
//#include "myvulkan/vulkanresource.h"
//#include "myvulkan/vulkanswapchain.h"
#include "render/font_render.h"

#include "core/camera.h"

struct GLFWwindow;


struct Image;

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

    virtual bool init(const char *windowStr, int screenWidth, int screenHeight);
    virtual void run() {}
    virtual void resized() {}
    //virtual bool startRender();

    // Returns offset to scratch buffer
    uint32_t updateRenderFrameBuffer();
    virtual void present(Image &presentImage);
    //virtual void recreateSwapchainData();
    //virtual bool createGraphics();
    //virtual void deleteFrameTargets();

    void resizeWindow(int w, int h);
    void setVsyncEnabled(bool enable);
    void setClearColor(float r, float g, float b, float a);
    void setTitle(const char *str);

    void checkCameraKeypresses(float dt, Camera& camera);

    double getDeltaTime();
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
/*
    Image mainColorRenderTarget;
    Image mainDepthRenderTarget;
    VkDebugUtilsMessengerEXT debugCallBack = 0;

    VkInstance instance = 0;
    VkSurfaceKHR surface = 0;
    VkPhysicalDevice physicalDevice = 0;

    DeviceWithQueues deviceWithQueues;

    Buffer scratchBuffer;
    Buffer renderFrameBuffer;

    VkRenderPass renderPass = 0;

    SwapChain swapchain;

    VkQueryPool queryPool = 0;

    VkSemaphore acquireSemaphore = 0;
    VkSemaphore releaseSemaphore = 0;

    VkFence fence = 0;
    bool waitForFence = true;
    VkCommandPool commandPool = 0;

    VkCommandBuffer commandBuffer = 0;
    VkFramebuffer targetFB = 0;


    VkPipelineCache pipelineCache = 0;
*/
};
