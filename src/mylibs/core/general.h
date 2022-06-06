#pragma once

#include <string_view>
#include <container/podvector.h>

#include <math/vector3.h>

bool loadBytes(std::string_view fileName, PodVector<char> &dataOut);
bool fileExists(std::string_view fileName);

// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a);
uint32_t getColor(const Vec4 &col);
