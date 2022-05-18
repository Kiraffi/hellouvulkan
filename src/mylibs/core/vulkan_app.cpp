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
    resized();
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
    vulk.initParams.vsync = vSync;
    vulk.needToResize = true;
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
    vkCmdResetQueryPool(vulk.commandBuffer, vulk.queryPool, 0, QUERY_COUNT);

    writeStamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    struct Buff
    {
        Vector2 areaSize;
        float tmp[6 + 8];
    };
    Buff buff{ Vector2(windowWidth, windowHeight) };
    addToCopylist(buff, vulk.renderFrameBufferHandle);

    fontSystem.update();
}


void VulkanApp::run()
{
    while (!glfwWindowShouldClose(window))
    {
        logicUpdate();
        if (startRender())
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
        //VK_CHECK(vkDeviceWaitIdle(vulk.device));

    }
    VK_CHECK(vkDeviceWaitIdle(vulk.device));
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

    if (keyDowns[GLFW_KEY_I].isDown)
    {
        camera.pitch -= rotationSpeed;
    }
    if (keyDowns[GLFW_KEY_K].isDown)
    {
        camera.pitch += rotationSpeed;
    }
    if (keyDowns[GLFW_KEY_J].isDown)
    {
        camera.yaw -= rotationSpeed;
    }
    if (keyDowns[GLFW_KEY_L].isDown)
    {
        camera.yaw += rotationSpeed;
    }

    camera.pitch = clamp(camera.pitch, -0.499f * PI, 0.4999f * PI);
    camera.yaw = ffmodf(camera.yaw, 2.0f * PI);

    Vec3 rightDir;
    Vec3 upDir;
    Vec3 forwardDir;

    camera.getCameraDirections(rightDir, upDir, forwardDir);


    if (keyDowns[GLFW_KEY_W].isDown)
    {
        camera.position = camera.position - forwardDir * moveSpeed;
    }
    if (keyDowns[GLFW_KEY_S].isDown)
    {
        camera.position = camera.position + forwardDir * moveSpeed;
    }
    if (keyDowns[GLFW_KEY_A].isDown)
    {
        camera.position = camera.position - rightDir * moveSpeed;
    }
    if (keyDowns[GLFW_KEY_D].isDown)
    {
        camera.position = camera.position + rightDir * moveSpeed;

    }
    if (keyDowns[GLFW_KEY_Q].isDown)
    {
        camera.position = camera.position - upDir * moveSpeed;
    }
    if (keyDowns[GLFW_KEY_E].isDown)
    {
        camera.position = camera.position + upDir * moveSpeed;
    }
}

uint32_t VulkanApp::writeStamp(VkPipelineStageFlagBits stage)
{
    uint32_t result = timeStampCount;
    vkCmdWriteTimestamp(vulk.commandBuffer, stage, vulk.queryPool, result);
    ++timeStampCount;
    return result;
}

static void printStats(VulkanApp& app)
{
    static uint32_t gpuframeCount = 0u;
    static double gpuTime = 0.0;
    static double cpuTimeStamp = app.getTime();

    if (app.timeStampCount < 2u)
        return;

    uint64_t queryResults[QUERY_COUNT];
    static constexpr size_t querySize = sizeof(uint64_t);
    VkResult res = (vkGetQueryPoolResults(vulk.device, vulk.queryPool,
        0, app.timeStampCount, querySize * app.timeStampCount, queryResults, querySize, VK_QUERY_RESULT_64_BIT));

    if (res != VK_SUCCESS)
        return;

    struct TimeValues
    {
        double timeDuration[QUERY_COUNT];
    };

    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(vulk.physicalDevice, &props);

    static TimeValues timeValues = {};
    for (uint32_t i = QUERY_COUNT - 1; i > 0; --i)
        timeValues.timeDuration[i] += (double(queryResults[i]) - double(queryResults[i - 1])) * props.limits.timestampPeriod * 1.0e-9f;

    gpuTime += (double(queryResults[app.timeStampCount - 1]) - double(queryResults[0])) * props.limits.timestampPeriod * 1.0e-9f;

    ++gpuframeCount;
    double currTime = app.getTime();

    double d = 1000.0 / gpuframeCount;
    double e = gpuframeCount;

    double cpuTime = currTime - cpuTimeStamp;
    app.gpuFps = e / gpuTime;
    app.cpuFps = e / cpuTime;

    if (currTime - cpuTimeStamp >= 1.0)
    {

        cpuTimeStamp += 1.0f;

        printf("Gpu: %.3fms, cpu: %.3fms.  GpuFps:%.1f, CpuFps:%.1f\n",
            float(gpuTime* d), (float)(cpuTime * d),
            app.gpuFps, app.cpuFps);
        gpuframeCount = 0u;

        for (uint32_t i = 0; i < QUERY_COUNT; ++i)
            timeValues.timeDuration[i] = 0.0;

        gpuTime = 0.0;

    }

    MouseState mouseState = app.getMouseState();

    char str[100];
    float fps = app.dt > 0.0 ? float(1.0 / app.dt) : 0.0f;

    sprintf(str, "%2.2fms, fps: %4.2f, gpuFps: %5.2f, mx: %i, my: %i, ml: %i, mr: %i, mb: %i",
        float(app.dt * 1000.0), app.cpuFps, app.gpuFps, mouseState.x, mouseState.y,
        mouseState.leftButtonDown, mouseState.rightButtonDown, mouseState.middleButtonDown
    );
    app.setTitle(str);
}