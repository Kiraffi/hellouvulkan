#pragma once

#include <core/mytypes.h>

struct MouseState
{
    i32 x;
    i32 y;
    bool leftButtonDown;
    bool rightButtonDown;
    bool middleButtonDown;
};

class InputApp
{
public:
    static bool init();
    static void reset();

    static bool isDown(i32 keyCode);
    static bool isPressed(i32 keyCode);
    static bool isReleased(i32 keyCode);

    static u32 getBufferedInput(i32 index);
    static u32 getBufferedInputCount();

    static MouseState getMouseState();
};
