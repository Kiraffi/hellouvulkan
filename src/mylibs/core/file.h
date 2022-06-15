#pragma once

#include <container/arraysliceview.h>
#include <container/podvector.h>

#include <string_view>

bool loadBytes(std::string_view filename, PodVector<char> &dataOut);
bool fileExists(std::string_view filename);

bool writeBytes(std::string_view filename, ArraySliceViewBytes bytes);
