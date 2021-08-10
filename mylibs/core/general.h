#pragma once

#include <string>
#include <vector>

bool loadBytes(const std::string& fileName, std::vector<char> &dataOut);

// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a);
