#pragma once

#include "core/mytypes.h"

struct Scene;

using myInitFunctionPtr = bool (*)(Scene& scene);
using myUpdateFunctionPtr = bool (*)(f32 deltaTime);

static constexpr i32 c_ScreenWidth = 1024;
static constexpr i32 c_ScreenHeight = 768;

static constexpr i32 c_ShadowWidth = 2048;
static constexpr i32 c_ShadowHeight = 2048;


void commonMain(const char* windowStr, i32 windowWidth, i32 windowHeight,
                myInitFunctionPtr initFunc,
                myUpdateFunctionPtr updateFunc);
