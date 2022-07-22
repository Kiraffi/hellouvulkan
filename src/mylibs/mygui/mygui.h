#pragma once

#include <core/mytypes.h>
#include <math/vector3.h>

bool initMyGui();
void deinitMyGui();

void startFrame();
void endFrame();

// USE STRING LITERALS ONLY
void window(const char *name);

void drawGui();

void guiColoredBox(const Vec2 &pos, const Vec2 &size, const Vec4 &color);
void guiText(const Vec2 &pos, const Vec2 &fontSize, const Vec4 &color, const char *txt);
