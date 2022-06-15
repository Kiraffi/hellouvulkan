#pragma once

#include <container/podvector.h>

#include <string_view>

bool loadBytes(std::string_view fileName, PodVector<char> &dataOut);
bool fileExists(std::string_view fileName);
