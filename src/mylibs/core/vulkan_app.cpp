#include "vulkan_app.h"

#if _WIN32
    #include <Windows.h> // begintimeperiod
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/camera.h>
#include <core/general.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>

#include <container/mymemory.h>

#include <math/general_math.h>
#include <math/matrix.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>


//#include <chrono>
#include <thread>
#include <string.h>


static double timer_frequency = 0.0;
static void printStats(VulkanApp& app);

static void error_callback(int error, const char* description)
{
    printf("Error: %s\n", description);
}




static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    VulkanApp *data = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    if(data)
    {
        data->resizeWindow(width, height);
        data->needToResize = true;
    }
}

static void keyboardHandlerCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    VulkanApp *data = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    if(!data)
        return;

    if(action == GLFW_PRESS)
    {
        if(key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose( window, 1 );
        else if (key >= 0 && key < 512)
        {
            data->keyDowns[ key ].isDown = true;
            ++data->keyDowns[ key ].pressCount;

            if (key >= 32 && key < 128)
            {
                char letter = ( char ) key;
                if (key >= 65 && key <= 90)
                {
                    int adder = 32;
                    if (( mods & ( GLFW_MOD_SHIFT | GLFW_MOD_CAPS_LOCK ) ) != 0)
                        adder = 0;
                    letter += adder;
                }
                data->bufferedPresses[ data->bufferedPressesCount ] = letter;
                ++data->bufferedPressesCount;
            }
        }
    }
    else if(action == GLFW_RELEASE && key >= 0 && key < 512)
    {
        data->keyDowns[ key ].isDown = false;
        ++data->keyDowns[ key ].pressCount;
    }
}



bool VulkanApp::init(const char *windowStr, int screenWidth, int screenHeight,
    const VulkanInitializationParameters &initParameters)
{
    initMemory();

    glfwSetErrorCallback(error_callback);
    int rc = glfwInit();
    ASSERT(rc);
    if (!rc)
    {
        printf("Couldn't initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    inited = true;

    window = glfwCreateWindow(screenWidth, screenHeight, windowStr, NULL, NULL);
    ASSERT(window);
    if (!window)
    {
        printf("Couldn't create glfw window\n");
        return false;
    }
    int w,h;
    glfwGetFramebufferSize(window, &w, &h);
    resizeWindow(w, h);

    if(!initVulkan(*this, initParameters))
    {
        printf("Failed to initialize vulkan\n");
        return false;
    }
    setVsync(initParameters.vsync);
    if (!fontSystem.init("assets/font/new_font.dat"))

    {
        printf("Failed to initialize the font system!\n");
        return false;
    }
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyboardHandlerCallback);
    glfwSetWindowUserPointer(window, this);

    camera.fovY = 90.0f;
    camera.zFar = 200.0f;
    camera.zNear = 0.125f;


    return true;

}

VulkanApp::~VulkanApp()
{
    fontSystem.deInit();
    deinitVulkan();

    if(window)
        glfwDestroyWindow(window);
    if(inited)
        glfwTerminate();
    window = nullptr;
}

void VulkanApp::resizeWindow(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
    //printf("Window size: %i: %i\n", w, h);
}

void VulkanApp::setVsync(VSyncType vSyncType)
{
    if (vSync == vSyncType)
        return;

    vSync = vSyncType;
    vulk->initParams.vsync = vSync;
    vulk->needToResize = true;
}

bool VulkanApp::isPressed(int keyCode)
{
    if (keyCode >= 0 && keyCode < 512)
        return keyDowns[ keyCode ].isDown && keyDowns[ keyCode ].pressCount > 0;
    return false;
}
bool VulkanApp::isReleased(int keyCode)
{
    if (keyCode >= 0 && keyCode < 512)
        return !keyDowns[ keyCode ].isDown && keyDowns[ keyCode ].pressCount > 0;
    return false;
}

bool VulkanApp::isDown(int keyCode)
{
    if (keyCode >= 0 && keyCode < 512)
        return keyDowns[ keyCode ].isDown;
    return false;
}

bool VulkanApp::isUp(int keyCode)
{
    if (keyCode >= 0 && keyCode < 512)
        return !keyDowns[ keyCode ].isDown;
    return false;
}

void VulkanApp::renderUpdate()
{
    timeStampCount = 0u;

    beginSingleTimeCommands();
    vkCmdResetQueryPool(vulk->commandBuffer, vulk->queryPool, 0, QUERY_COUNT);

    writeStamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    vulkanResourceFrameUpdate();

    struct FrameBuffer
    {
        Matrix camMat;
        Matrix viewProj;
        Matrix mvp;
        Matrix sunMatrix;

        Vector2 areaSize;
        Vector2 tmp1;

        Vector4 camPos;
        Vector4 tmp3;
        Vector4 tmp4;

        Matrix inverseMvp;
    };
    FrameBuffer frameBufferData;


    sunCamera.calculateOrtographicPosition(camera.position);
    const SwapChain& swapchain = vulk->swapchain;
    camera.aspectRatioWByH = float(swapchain.width) / float(swapchain.height);

    if (useSunCamera)
    {
        frameBufferData.camMat = sunCamera.getCameraMatrix();
        frameBufferData.viewProj = sunCamera.ortographicProjection(50.0f, 50.0f);
    }
    else
    {
        frameBufferData.camMat = camera.getCameraMatrix();
        frameBufferData.viewProj = camera.perspectiveProjection();
    }
    frameBufferData.mvp = frameBufferData.viewProj * frameBufferData.camMat;
    frameBufferData.inverseMvp = inverse(frameBufferData.mvp);

    frameBufferData.sunMatrix = sunCamera.ortographicProjection(50.0f, 50.0f) * sunCamera.getCameraMatrix();
    frameBufferData.camPos = Vector4(camera.position, 0.0f);
    frameBufferData.areaSize = Vector2(windowWidth, windowHeight);
    addToCopylist(frameBufferData, vulk->renderFrameBufferHandle);

    fontSystem.update();
}


void VulkanApp::run()
{
    while (!glfwWindowShouldClose(window))
    {
        logicUpdate();
        bool resizeHappen = !startRender();

        if (!resizeHappen)
        {
            renderUpdate();
            renderDraw();
            printStats(*this);
        }
        defragMemory();

        #if _WIN32
            timeBeginPeriod(1);
        #endif
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        #if _WIN32
            timeEndPeriod(1);
        #endif
        // needed cos of resize?
        if(resizeHappen)
            VK_CHECK(vkDeviceWaitIdle(vulk->device));

    }
    VK_CHECK(vkDeviceWaitIdle(vulk->device));
}

void VulkanApp::logicUpdate()
{
    fontSystem.reset();

    for (int i = 0; i < ARRAYSIZES(keyDowns); ++i)
    {
        keyDowns[ i ].pressCount = 0u;
    }

    bufferedPressesCount = 0u;
    dt = timer.getLapDuration();

    glfwPollEvents();


    if (isPressed(GLFW_KEY_C))
    {
        useSunCamera = !useSunCamera;
    }
}

void VulkanApp::setTitle(const char *str)
{
    glfwSetWindowTitle(window, str);
}

double VulkanApp::getDeltaTime()
{
    return dt;
}

double VulkanApp::getTime()
{
    return glfwGetTime();
}

MouseState VulkanApp::getMouseState()
{
    MouseState mouseState;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mouseState.x = xpos;
    mouseState.y = ypos;
    mouseState.y = mouseState.y;

    mouseState.leftButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    mouseState.rightButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    mouseState.middleButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    return mouseState;
}




void VulkanApp::checkCameraKeypresses(float deltaTime, Camera &camera)
{
    //printf("deltatime: %f\n", deltaTime);
    float moveBooster = keyDowns[GLFW_KEY_LEFT_SHIFT].isDown ? 5.0f : 1.0f;
    float rotationBooster = keyDowns[GLFW_KEY_LEFT_SHIFT].isDown ? 2.0f : 1.0f;

    float moveSpeed = deltaTime * 2.0f * moveBooster;
    float rotationSpeed = deltaTime * 1.0f * rotationBooster;

    if (isDown(GLFW_KEY_I))
    {
        camera.pitch += rotationSpeed;
    }
    if (isDown(GLFW_KEY_K))
    {
        camera.pitch -= rotationSpeed;
    }
    if (isDown(GLFW_KEY_J))
    {
        camera.yaw += rotationSpeed;
    }
    if (isDown(GLFW_KEY_L))
    {
        camera.yaw -= rotationSpeed;
    }
    /*
    if (isDown(GLFW_KEY_O))
    {
        camera.roll += rotationSpeed;
    }
    if (isDown(GLFW_KEY_P))
    {
        camera.roll -= rotationSpeed;
    }
    */
    camera.pitch = clamp(camera.pitch, -0.499f * PI, 0.4999f * PI);
    camera.yaw = ffmodf(camera.yaw, 2.0f * PI);
    camera.roll = ffmodf(camera.roll, 2.0f * PI);

    Vec3 rightDir;
    Vec3 upDir;
    Vec3 forwardDir;

    getDirectionsFromPitchYawRoll(camera.pitch, camera.yaw, camera.roll, rightDir, upDir, forwardDir);


    if (isDown(GLFW_KEY_W))
    {
        camera.position = camera.position - forwardDir * moveSpeed;
    }
    if (isDown(GLFW_KEY_S))
    {
        camera.position = camera.position + forwardDir * moveSpeed;
    }
    if (isDown(GLFW_KEY_A))
    {
        camera.position = camera.position - rightDir * moveSpeed;
    }
    if (isDown(GLFW_KEY_D))
    {
        camera.position = camera.position + rightDir * moveSpeed;

    }
    if (isDown(GLFW_KEY_Q))
    {
        camera.position = camera.position - upDir * moveSpeed;
    }
    if (isDown(GLFW_KEY_E))
    {
        camera.position = camera.position + upDir * moveSpeed;
    }
}

uint32_t VulkanApp::writeStamp(VkPipelineStageFlagBits stage)
{
    uint32_t result = timeStampCount;
    vkCmdWriteTimestamp(vulk->commandBuffer, stage, vulk->queryPool, result);
    ++timeStampCount;
    return result;
}

static void printStats(VulkanApp& app)
{
    static uint32_t gpuframeCount = 0u;
    static uint32_t cpuframeCount = 0u;
    static double gpuTime = 0.0;
    static double cpuTimeStamp = app.getTime();
    
    struct TimeValues
    {
        double timeDuration[QUERY_COUNT];
    };
    static TimeValues timeValues = {};

    if (app.timeStampCount < 2u)
        return;

    uint64_t queryResults[QUERY_COUNT];
    static constexpr size_t querySize = sizeof(uint64_t);
    VkResult res = (vkGetQueryPoolResults(vulk->device, vulk->queryPool,
        0, app.timeStampCount, querySize * app.timeStampCount, queryResults, querySize, VK_QUERY_RESULT_64_BIT));

    if (res == VK_SUCCESS)
    {
        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(vulk->physicalDevice, &props);

  
        for (uint32_t i = QUERY_COUNT - 1; i > 0; --i)
            timeValues.timeDuration[i] += (double(queryResults[i]) - double(queryResults[i - 1])) * props.limits.timestampPeriod * 1.0e-9f;

        gpuTime += (double(queryResults[app.timeStampCount - 1]) - double(queryResults[0])) * props.limits.timestampPeriod * 1.0e-9f;
        ++gpuframeCount;
    }
    ++cpuframeCount;

    double currTime = app.getTime();

    double cpuTime = currTime - cpuTimeStamp;
    app.gpuFps = gpuframeCount / gpuTime;
    app.cpuFps = cpuframeCount / cpuTime;

    if (currTime - cpuTimeStamp >= 1.0)
    {

        cpuTimeStamp += 1.0f;

        printf("Gpu: %4.3fms, cpu: %4.3fms.  GpuFps:%4.2f, CpuFps:%4.2f\n",
            float(gpuTime* 1000.0 / gpuframeCount), (float)(cpuTime * 1000.0f / cpuframeCount),
            app.gpuFps, app.cpuFps);

        for (uint32_t i = 0; i < QUERY_COUNT; ++i)
            timeValues.timeDuration[i] = 0.0;
            
        gpuTime = 0.0;
        gpuframeCount = 0u;
        cpuframeCount = 0u;
    }

    MouseState mouseState = app.getMouseState();

    char str[100];
    float fps = app.dt > 0.0 ? float(1.0 / app.dt) : 0.0f;

    sprintf(str, "%3.2fms, fps: %4.2f, gpuFps: %5.2f, mx: %i, my: %i, ml: %i, mr: %i, mb: %i",
        float(app.dt * 1000.0), app.cpuFps, app.gpuFps, mouseState.x, mouseState.y,
        mouseState.leftButtonDown, mouseState.rightButtonDown, mouseState.middleButtonDown
    );
    app.setTitle(str);
}