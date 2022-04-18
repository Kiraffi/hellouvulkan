#pragma once

#include <stdint.h>

struct Memory;

void initMemory();

[[nodiscard]] Memory allocateMemoryBytes(uint32_t size);
bool deAllocateMemory(Memory memory);
[[nodiscard]] Memory resizeMemory(Memory memory, uint32_t size);

void defragMemory();

[[nodiscard]] bool isValidMemory(Memory memory);
[[nodiscard]] uint8_t *getMemoryBegin(Memory memory);
[[nodiscard]] uint8_t *getMemoryEnd(Memory memory);

struct MemoryHandle
{
    uint32_t value = ~0u;
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
MemoryType<T> allocateMemory(uint32_t count)
{
    uint32_t size = sizeof(T);
    return allocateMemoryBytes(size * count);
}
