#pragma once

#include <container/podvector.h>
#include <core/mytypes.h>

struct Buffer;
struct UniformBufferManager;

using MemoryIndexType = uint64_t;

struct UniformBufferHandle
{
    UniformBufferManager* manager = nullptr;
    uint32_t index = 0u;
    uint32_t size = 0u;
    // 64M / 64k = 64 * 1024 * 1024 / (64 * 1024) = 1024;
    bool isValid() const { return index <= 1024 && index > 0; }
    uint32_t getOffset() const { ASSERT(isValid()); return (index - 1u) * size; }
};

struct UniformBufferManager
{
    // 64M / 64k = 64 * 1024 * 1024 / (64 * 1024) = 1024;
    void init(Buffer &buf);

    void freeHandle(UniformBufferHandle handle);
    UniformBufferHandle reserveHandle();

    // 1 bit per allocation, so index 0 bit 7 is 8th allocation.
    PodVector<MemoryIndexType> usedIndices;
    Buffer *buffer;
    uint32_t allocationCount = 0;
    bool inited = false;
};