#pragma once

#include <string_view>
#include <container/podvector.h>

bool loadBytes(std::string_view fileName, PodVector<char> &dataOut);

// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a);
