#pragma once

#include <container/bytebuffer.h>

bool loadBytes(const char *filename, ByteBuffer &dataOut);
bool fileExists(const char *filename);

bool writeBytes(const char *filename, const ByteBuffer &data);
