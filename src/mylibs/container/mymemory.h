#pragma once

#include <stdint.h>

struct Memory;

void initMemory();
Memory allocateMemoryBytes(uint32_t size);

bool deAllocate(Memory memory);

void defragMemory();
bool isValidMemory(Memory memory);


struct MemoryHandle
{
    uint32_t value;
};

struct Memory
{
    MemoryHandle handle;
};

struct MemoryAutoRelease
{
    MemoryAutoRelease(Memory m) : mem(m) {}
    ~MemoryAutoRelease() { deAllocate(mem); }
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
    ~MemoryAutoReleaseType() { deAllocate(mem.mem); }
    MemoryType<T> mem;
};

template<class T>
MemoryType<T> allocateMemory(uint32_t count)
{
    uint32_t size = sizeof(T);
    return allocateMemoryBytes(size * count);
}
