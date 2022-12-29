#include "vulkan_app.h"

#include <app/inputapp.h>

#include <core/assert.h>
#include <core/mytypes.h>

#if WIN32
    #include <Windows.h> // begintimeperiod
    #include <timeapi.h>
#else
    #include <unistd.h>
#endif

#include <resources/globalresources.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static WindowApp sWindowApp;
static GLFWwindow* sWindow = nullptr;

static void sResizeWindow(i32 width, i32 height);

static void sErrorCB(i32 error, const char* description)
{
    printf("Error: %s\n", description);
}

static void sFrameBufCB(GLFWwindow* window, i32 width, i32 height)
{
    sResizeWindow(width, height);
}

static void sResizeWindow(i32 width, i32 height)
{
    if(width != sWindowApp.windowWidth || height != sWindowApp.windowHeight)
    {
        sWindowApp.resized = true;
    }
    sWindowApp.viewportWidth = width;
    sWindowApp.viewportHeight = height;
    sWindowApp.windowWidth = width;
    sWindowApp.windowHeight = height;
}

bool VulkanApp::initApp(const char *windowStr, i32 screenWidth, i32 screenHeight)
{
    //if (!initGlobalResources())
    //{
    //    return false;
    //}

    sWindowApp.windowWidth = screenWidth;
    sWindowApp.windowHeight = screenHeight;

    glfwSetErrorCallback(sErrorCB);
    i32 rc = glfwInit();
    ASSERT(rc);
    if (!rc)
    {
        printf("Couldn't initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    sWindowApp.inited = true;

    sWindow = glfwCreateWindow(screenWidth, screenHeight, windowStr, NULL, NULL);
    ASSERT(sWindow);
    if (!sWindow)
    {
        printf("Couldn't create glfw window\n");
        return false;
    }

    i32 w, h;
    glfwGetFramebufferSize(sWindow, &w, &h);

    sResizeWindow(w, h);
    glfwSetFramebufferSizeCallback(sWindow, sFrameBufCB);

    return true;
}

void VulkanApp::deinitApp()
{
    //deinitGlobalResources();
    if(sWindow)
        glfwDestroyWindow(sWindow);
    if(sWindowApp.inited)
        glfwTerminate();
    sWindow = nullptr;
}

const WindowApp& VulkanApp::getWindowApp()
{
    return sWindowApp;
}

const GLFWwindow* VulkanApp::getWindow()
{
    return sWindow;
}

GLFWwindow* VulkanApp::getWindowRef()
{
    return sWindow;
}

void VulkanApp::setTitle(const char *str)
{
    ASSERT(sWindow);
    glfwSetWindowTitle(sWindow, str);
}

void VulkanApp::setWindowPosition(u32 x, u32 y)
{
    ASSERT(sWindow);
    glfwSetWindowPos(sWindow, x, y);
}

bool VulkanApp::updateApp()
{
    double currTime = glfwGetTime();
    if(glfwWindowShouldClose(sWindow))
    {
        return false;
    }
    InputApp::reset();
    glfwPollEvents();
    sWindowApp.frameDt = currTime - sWindowApp.appRuntime;
    sWindowApp.appRuntime = currTime;
    return true;
}

void VulkanApp::frameEnd()
{
    static constexpr u32 SleepDuration = 5;
    #if WIN32
        timeBeginPeriod(1);
        Sleep(SleepDuration);
    #else
        usleep(SleepDuration * 1000);
    #endif
        //std::this_thread::sleep_for(std::chrono::milliseconds(5));
    #if WIN32
        timeEndPeriod(1);
    #endif
}

#if 0
bool VulkanApp::updateApp()
{
    defragMemory();
    if(glfwWindowShouldClose(sWindow))
    {
        return false;
    }
    InputApp::reset();
    glfwPollEvents();

    return true;
}

        logicUpdate();
        bool resizeHappen = !startRender();

        if (!resizeHappen)
        {
            updateStats(*this);
            vulk->currentStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            renderUpdate();
            renderDraw();
            printStats(*this);
        }
        defragMemory();
        static constexpr u32 SleepDuration = 5;
        #if WIN32
            timeBeginPeriod(1);
            Sleep(SleepDuration);
        #else
            usleep(SleepDuration * 1000);
        #endif
            //std::this_thread::sleep_for(std::chrono::milliseconds(5));
        #if WIN32
            timeEndPeriod(1);
        #endif
        // needed cos of resize?
        if(resizeHappen)
            VK_CHECK(vkDeviceWaitIdle(vulk->device));

    }
    VK_CHECK(vkDeviceWaitIdle(vulk->device));

}

bool VulkanApp::init(const char *windowStr, i32 screenWidth, i32 screenHeight)
{
    if(!initGlobalResources())
        return false;

    glfwSetErrorCallback(error_callback);
    i32 rc = glfwInit();
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
    i32 w,h;
    glfwGetFramebufferSize(window, &w, &h);
    resizeWindow(w, h);

    if(!initVulkan(*this))
    {
        printf("Failed to initialize vulkan\n");
        return false;
    }
    setVsync(VulkanInitializationParameters::get().vsync);
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

    sunCamera.width = 50.0f;
    sunCamera.height = 50.0f;
    sunCamera.cameraType = CameraType::ORTHO;

    return true;

}

VulkanApp::~VulkanApp()
{
    fontSystem.deInit();
    deinitVulkan();
    deinitGlobalResources();

    if(window)
        glfwDestroyWindow(window);
    if(inited)
        glfwTerminate();
    window = nullptr;
}

void VulkanApp::resizeWindow(i32 w, i32 h)
{
    viewportWidth = w;
    viewportHeight = h;
    windowWidth = w;
    windowHeight = h;
}

void VulkanApp::setVsync(VSyncType vSyncType)
{
    if (vSync == vSyncType)
        return;

    vSync = vSyncType;
    auto &initParams = VulkanInitializationParameters::getRef();
    initParams.vsync = vSync;
    vulk->needToResize = true;
}

bool VulkanApp::isPressed(i32 keyCode) const
{
    if (keyCode >= 0 && keyCode < 512)
        return keyDowns[ keyCode ].isDown && keyDowns[ keyCode ].pressCount > 0;
    return false;
}
bool VulkanApp::isReleased(i32 keyCode) const
{
    if (keyCode >= 0 && keyCode < 512)
        return !keyDowns[ keyCode ].isDown && keyDowns[ keyCode ].pressCount > 0;
    return false;
}

bool VulkanApp::isDown(i32 keyCode) const
{
    if (keyCode >= 0 && keyCode < 512)
        return keyDowns[ keyCode ].isDown;
    return false;
}

bool VulkanApp::isUp(i32 keyCode) const
{
    if (keyCode >= 0 && keyCode < 512)
        return !keyDowns[ keyCode ].isDown;
    return false;
}

void VulkanApp::renderUpdate()
{
    vulk->queryPoolIndexCounts[vulk->frameIndex] = 0u;

    //beginSingleTimeCommands();
    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(vulk->commandBuffer, &beginInfo));

    vkCmdResetQueryPool(vulk->commandBuffer, vulk->queryPools[vulk->frameIndex], 0, QUERY_COUNT);

    writeStamp();
    VulkanResources::update();

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

    // bit ugly.......
    sunCamera.calculateOrtographicPosition(camera.position);
    if(windowWidth > 0 && windowHeight > 0)
        camera.updateCameraState(windowWidth, windowHeight);
    sunCamera.updateCameraState(50.0f, 50.0f);

    if (useSunCamera)
    {
        frameBufferData.mvp = sunCamera.worldToViewMat;
        frameBufferData.inverseMvp = sunCamera.viewToWorldMat;
    }
    else
    {
        frameBufferData.mvp = camera.worldToViewMat;
        frameBufferData.inverseMvp = camera.viewToWorldMat;
    }
    frameBufferData.sunMatrix = sunCamera.worldToViewMat;
    frameBufferData.camPos = Vector4(camera.position, 0.0f);
    frameBufferData.areaSize = getWindowSize();
    addToCopylist(frameBufferData, vulk->renderFrameBufferHandle[vulk->frameIndex]);

    fontSystem.update();
}


void VulkanApp::run()
{
    defragMemory();
    while (!glfwWindowShouldClose(window))
    {
        logicUpdate();
        bool resizeHappen = !startRender();

        if (!resizeHappen)
        {
            updateStats(*this);
            vulk->currentStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            renderUpdate();
            renderDraw();
            printStats(*this);
        }
        defragMemory();
        static constexpr u32 SleepDuration = 5;
        #if WIN32
            timeBeginPeriod(1);
            Sleep(SleepDuration);
        #else
            usleep(SleepDuration * 1000);
        #endif
            //std::this_thread::sleep_for(std::chrono::milliseconds(5));
        #if WIN32
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

    for (i32 i = 0; i < ARRAYSIZES(keyDowns); ++i)
    {
        keyDowns[ i ].pressCount = 0u;
    }

    bufferedPressesCount = 0u;
    dt = timer.getLapDuration();

    glfwPollEvents();


    if (isPressed(GLFW_KEY_RIGHT_BRACKET))
    {
        useSunCamera = !useSunCamera;
    }
}


const Camera &VulkanApp::getActiveCamera() const
{
    if(useSunCamera)
        return sunCamera;
    return camera;
}

Vec2 VulkanApp::getWindowSize() const
{
    return Vec2(windowWidth, windowHeight);
}

double VulkanApp::getDeltaTime() const
{
    return dt;
}

double VulkanApp::getTime() const
{
    return glfwGetTime();
}

MouseState VulkanApp::getMouseState() const
{
    MouseState mouseState{};

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

    if (isDown(GLFW_KEY_O))
    {
        camera.roll += rotationSpeed;
    }
    if (isDown(GLFW_KEY_P))
    {
        camera.roll -= rotationSpeed;
    }

    camera.pitch = Supa::clampf(camera.pitch, -0.499f * PI, 0.4999f * PI);
    camera.yaw = Supa::modf(camera.yaw, 2.0f * PI);
    camera.roll = Supa::modf(camera.roll, 2.0f * PI);

    Vec3 rightDir;
    Vec3 upDir;
    Vec3 forwardDir;

    // camera forward = -forward.
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

static void updateStats(VulkanApp &app)
{
    static u32 gpuframeCount = 0u;
    static u32 cpuframeCount = 0u;
    static double gpuTime = 0.0;
    static double cpuTimeStamp = app.getTime();

    struct TimeValues
    {
        double timeDuration[QUERY_COUNT];
    };
    static TimeValues timeValues = {};
    u32 oldPoolIndex = vulk->frameIndex;
    u32 timeStamps = vulk->queryPoolIndexCounts[oldPoolIndex];

    ++cpuframeCount;
    if(timeStamps < 2u)
        return;

    u64 queryResults[QUERY_COUNT];
    static constexpr size_t querySize = sizeof(u64);
    VkQueryPool queryPool = vulk->queryPools[oldPoolIndex];
    VkResult res = (vkGetQueryPoolResults(vulk->device, queryPool,
        0, timeStamps, querySize * timeStamps, queryResults, querySize, VK_QUERY_RESULT_64_BIT));

    if(res == VK_SUCCESS)
    {
        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(vulk->physicalDevice, &props);


        for(u32 i = QUERY_COUNT - 1; i > 0; --i)
            timeValues.timeDuration[i] += (double(queryResults[i]) - double(queryResults[i - 1])) * props.limits.timestampPeriod * 1.0e-9f;

        gpuTime += (double(queryResults[timeStamps - 1]) - double(queryResults[0])) * props.limits.timestampPeriod * 1.0e-9f;
        ++gpuframeCount;
    }

    double currTime = app.getTime();

    double cpuTime = currTime - cpuTimeStamp;
    app.gpuFps = gpuframeCount / gpuTime;
    app.cpuFps = cpuframeCount / cpuTime;
    app.cpudt = cpuTime / cpuframeCount * 1000.0f;
    app.gpudt = gpuTime / gpuframeCount * 1000.0f;
    if(currTime - cpuTimeStamp >= 1.0)
    {
        while(currTime - cpuTimeStamp >= 1.0f)
            cpuTimeStamp += 1.0f;

        printf("Gpu: %4.3fms, cpu: %4.3fms.  GpuFps:%4.2f, CpuFps:%4.2f\n",
            float(gpuTime * 1000.0 / gpuframeCount), (float)(cpuTime * 1000.0f / cpuframeCount),
            app.gpuFps, app.cpuFps);

        for(u32 i = 0; i < QUERY_COUNT; ++i)
            timeValues.timeDuration[i] = 0.0;

        gpuTime = 0.0;
        gpuframeCount = 0u;
        cpuframeCount = 0u;
    }
}

static void printStats(VulkanApp &app)
{
    MouseState mouseState = app.getMouseState();

    char str[100];
    float fps = app.dt > 0.0 ? float(1.0 / app.dt) : 0.0f;

    snprintf(str, 100, "cpu: %3.2fms, cpuFps: %4.2f, gpu: %3.2fms, gpuFps: %5.2f, mx: %i, my: %i, ml: %i, mr: %i, mb: %i",
        app.cpudt, app.cpuFps, app.gpudt, app.gpuFps, mouseState.x, mouseState.y,
        mouseState.leftButtonDown, mouseState.rightButtonDown, mouseState.middleButtonDown
    );
    app.setTitle(str);
}

#endif