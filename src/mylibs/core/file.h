#pragma once

#include <container/string.h>
#include <container/stringview.h>

bool loadBytes(const char *filename, String &dataOut);
bool fileExists(const char *filename);

bool writeBytes(const char *filename, StringView bytes);
