#include "vulkan_app.h"

#if _MSC_VER
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


#include <chrono>
#include <thread>
#include <string.h>


static double timer_frequency = 0.0;

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

void VulkanApp::setVsyncEnabled(bool enable)
{
    vSync = enable;
    // Use v-sync
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

void VulkanApp::updateRenderFrameBuffer()
{
    struct Buff
    {
        Vector2 areaSize;
        float tmp[6 + 8];
    };

    Buff buff{ Vector2(windowWidth, windowHeight) };

    // use scratch buffer to unifrom buffer transfer
    uint32_t buffSize = uint32_t(sizeof(Buff));
    memcpy(((uint8_t *)vulk.scratchBuffer.data) + vulk.scratchBufferOffset, &buff, buffSize);
    {
        VkBufferCopy region = { 
            vulk.scratchBufferOffset,
            vulk.renderFrameBufferHandle.getOffset(), 
            VkDeviceSize(buffSize)
        };
        vkCmdCopyBuffer(vulk.commandBuffer, vulk.scratchBuffer.buffer, vulk.uniformBuffer.buffer, 1, &region);
    }

    VkBufferMemoryBarrier bar[]
    {
        bufferBarrier(vulk.uniformBuffer.buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffSize),
    };

    vkCmdPipelineBarrier(vulk.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, bar, 0, nullptr);
    
    vulk.scratchBufferOffset += buffSize;

    fontSystem.update();
}


void VulkanApp::run()
{
    while (!glfwWindowShouldClose(window))
    {
        update();
        defragMemory();

        #if _MSC_VER
            timeBeginPeriod(1);
        #endif
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        #if _MSC_VER
            timeEndPeriod(1);
        #endif
        //VK_CHECK(vkDeviceWaitIdle(vulk.device));

    }
    VK_CHECK(vkDeviceWaitIdle(vulk.device));
}

void VulkanApp::update()
{
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
