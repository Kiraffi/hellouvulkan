#pragma once

#include <core/mytypes.h>

struct Memory;

void initMemory();
void deinitMemory();

[[nodiscard]] Memory allocateMemoryBytes(u32 size);
bool deAllocateMemory(Memory memory);
[[nodiscard]] Memory resizeMemory(Memory memory, u32 size);

void defragMemory();

[[nodiscard]] bool isValidMemory(Memory memory);
[[nodiscard]] u8 *getMemoryBegin(Memory memory);
[[nodiscard]] u8 *getMemoryEnd(Memory memory);

struct MemoryHandle
{
    u32 value = ~0u;
};

struct Memory
{
    MemoryHandle handle;
};

struct MemoryAutoRelease
{
    MemoryAutoRelease(Memory m) : mem(m) {}
    ~MemoryAutoRelease() { deAllocateMemory(mem); }
    Memory mem;
};



/*
// Some templated versions for memory per struct type
template<class T>
struct MemoryType
{
    MemoryType(Memory m) : mem{m} {}
    Memory mem;
};

template<class T>
struct MemoryAutoReleaseType
{
    MemoryAutoReleaseType(MemoryType<T> m) : mem(m) {}
    ~MemoryAutoReleaseType() { deAllocateMemory(mem.mem); }
    MemoryType<T> mem;
};

template<class T>
MemoryType<T> allocateMemory(u32 count)
{
    u32 size = sizeof(T);
    return allocateMemoryBytes(size * count);
}
*/