#pragma once

#include <container/arraysliceview.h>
#include <container/podvector.h>

bool loadBytes(const char *filename, PodVector<char> &dataOut);
bool fileExists(const char *filename);

bool writeBytes(const char *filename, ArraySliceViewBytes bytes);
