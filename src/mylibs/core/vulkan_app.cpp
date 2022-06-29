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
#include <math/matrix_inline_functions.h>
#include <math/quaternion_inline_functions.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>

#include <resources/globalresources.h>

//#include <chrono>
#include <thread>
#include <string.h>


static double timer_frequency = 0.0;
static void printStats(VulkanApp& app);
static void updateStats(VulkanApp &app);

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
        /*
        if(key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose( window, 1 );
        */
        if (key >= 0 && key < 512)
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
    if(!initGlobalResources())
        return false;

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

void VulkanApp::resizeWindow(int w, int h)
{
    viewportWidth = w;
    viewportHeight = h;
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

bool VulkanApp::isPressed(int keyCode) const
{
    if (keyCode >= 0 && keyCode < 512)
        return keyDowns[ keyCode ].isDown && keyDowns[ keyCode ].pressCount > 0;
    return false;
}
bool VulkanApp::isReleased(int keyCode) const
{
    if (keyCode >= 0 && keyCode < 512)
        return !keyDowns[ keyCode ].isDown && keyDowns[ keyCode ].pressCount > 0;
    return false;
}

bool VulkanApp::isDown(int keyCode) const
{
    if (keyCode >= 0 && keyCode < 512)
        return keyDowns[ keyCode ].isDown;
    return false;
}

bool VulkanApp::isUp(int keyCode) const
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

        #if _WIN32
            timeBeginPeriod(1);
        #endif
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
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

void VulkanApp::setWindowPosition(uint32_t x, uint32_t y)
{
    glfwSetWindowPos(window, x, y);
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

    camera.pitch = clamp(camera.pitch, -0.499f * PI, 0.4999f * PI);
    camera.yaw = ffmodf(camera.yaw, 2.0f * PI);
    camera.roll = ffmodf(camera.roll, 2.0f * PI);

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
    static uint32_t gpuframeCount = 0u;
    static uint32_t cpuframeCount = 0u;
    static double gpuTime = 0.0;
    static double cpuTimeStamp = app.getTime();

    struct TimeValues
    {
        double timeDuration[QUERY_COUNT];
    };
    static TimeValues timeValues = {};
    uint32_t oldPoolIndex = vulk->frameIndex;
    uint32_t timeStamps = vulk->queryPoolIndexCounts[oldPoolIndex];

    ++cpuframeCount;
    if(timeStamps < 2u)
        return;

    uint64_t queryResults[QUERY_COUNT];
    static constexpr size_t querySize = sizeof(uint64_t);
    VkQueryPool queryPool = vulk->queryPools[oldPoolIndex];
    VkResult res = (vkGetQueryPoolResults(vulk->device, queryPool,
        0, timeStamps, querySize * timeStamps, queryResults, querySize, VK_QUERY_RESULT_64_BIT));

    if(res == VK_SUCCESS)
    {
        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(vulk->physicalDevice, &props);


        for(uint32_t i = QUERY_COUNT - 1; i > 0; --i)
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

        for(uint32_t i = 0; i < QUERY_COUNT; ++i)
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