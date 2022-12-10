#include "inputapp.h"

#include <app/vulkan_app.h>

#include <core/assert.h>
#include <core/mytypes.h>
#include <core/general.h>

#include <GLFW/glfw3.h>

static const i32 KEYCOUNT = 512;

struct KeyState
{
    bool isDown = false;
    u32 pressCount = 0u;
};

struct Inputs
{
    KeyState keyDowns[KEYCOUNT] = {};
    u32 bufferedPresses[128] = {};
    u32 bufferedPressesCount = 0u;
};

Inputs sInputs;

static void sKBHandlerCB(GLFWwindow *window, i32 key, i32 scancode, i32 action, i32 mods)
{
    if(action == GLFW_PRESS)
    {
        /*
        if(key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose( window, 1 );
        */
        if (key >= 0 && key < KEYCOUNT)
        {
            sInputs.keyDowns[ key ].isDown = true;
            ++sInputs.keyDowns[ key ].pressCount;

            if (key >= 32 && key < 128)
            {
                char letter = ( char ) key;
                if (key >= 65 && key <= 90)
                {
                    i32 adder = 32;
                    if (( mods & ( GLFW_MOD_SHIFT | GLFW_MOD_CAPS_LOCK ) ) != 0)
                        adder = 0;
                    letter += adder;
                }
                sInputs.bufferedPresses[ sInputs.bufferedPressesCount ] = letter;
                ++sInputs.bufferedPressesCount;
            }
        }
    }
    else if(action == GLFW_RELEASE && key >= 0 && key < KEYCOUNT)
    {
        sInputs.keyDowns[ key ].isDown = false;
        ++sInputs.keyDowns[ key ].pressCount;
    }
}


bool InputApp::init()
{
    GLFWwindow* window = VulkanApp::getWindowRef();
    ASSERT(window);
    glfwSetKeyCallback(window, sKBHandlerCB);
    return window != nullptr;
}

void InputApp::reset()
{
    for (i32 i = 0; i < KEYCOUNT; ++i)
    {
        sInputs.keyDowns[ i ].pressCount = 0u;
    }

    sInputs.bufferedPressesCount = 0u;
}

bool InputApp::isPressed(i32 keyCode)
{
    if (keyCode >= 0 && keyCode < KEYCOUNT)
    {
        return sInputs.keyDowns[ keyCode ].isDown
            && sInputs.keyDowns[ keyCode ].pressCount > 0;
    }
    return false;
}

bool InputApp::isReleased(i32 keyCode)
{
    if (keyCode >= 0 && keyCode < KEYCOUNT)
    {
        return !sInputs.keyDowns[ keyCode ].isDown
            && sInputs.keyDowns[ keyCode ].pressCount > 0;
    }
    return false;
}

bool InputApp::isDown(i32 keyCode)
{
    if (keyCode >= 0 && keyCode < KEYCOUNT)
    {
        return sInputs.keyDowns[ keyCode ].isDown;
    }
    return false;
}

u32 InputApp::getBufferedInput(i32 index)
{
    if(index < sInputs.bufferedPressesCount)
    {
        return sInputs.bufferedPresses[index];
    }
    return ~0u;
}

u32 InputApp::getBufferedInputCount()
{
    return sInputs.bufferedPressesCount;
}

MouseState InputApp::getMouseState()
{
    GLFWwindow* window = VulkanApp::getWindowRef();
    ASSERT(window);
    MouseState mouseState{};

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mouseState.x = xpos;
    mouseState.y = ypos;

    mouseState.leftButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    mouseState.rightButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    mouseState.middleButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    return mouseState;
}

