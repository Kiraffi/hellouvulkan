#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mymemory.h"
#include "core/mytypes.h"

///// THIS IS NOT TO WORK WITH MULTIPLE THREADS!
static constexpr uint32_t MaxAllocations = 1024;
static constexpr uint32_t MaxMemorySize = 1024 * 1024 * 1024;
static constexpr uint32_t AllocatedSize = MaxMemorySize + 65536u;

static constexpr uint32_t MinimumMemoryChunkSize = 256u;
static constexpr uint32_t MemoryAlignment = 4096u;

// What to initialize originally the memory, probably not really useful, since
// it would have to initialize that during freeing.
static constexpr uint8_t DebugValue = 0xabu;

struct MemoryArea
{
    uint32_t startIndex = 0;
    uint32_t size = 0;
};

struct AllMemory
{
    ~AllMemory()
    {
        printf("Deleting memory!\n");
        if(allocationCount > 0)
            printf("Allocations alive: %u\n", allocationCount);

        delete[] memory;
    }

    uint8_t *memory = nullptr;
    uint8_t *memoryStart = nullptr;

    MemoryArea memoryAreas[MaxAllocations] = {};
    uint32_t freedAllocationIndices[MaxAllocations] = {};
    uint32_t usedAllocationIndices[MaxAllocations] = {};

    uint32_t handleIterations[MaxAllocations] = {};

    uint32_t freedAllocationCount = 0;
    uint32_t allocationCount = 0;

    uint32_t memoryPointer = 0;

    bool inited = false;
};

static AllMemory allMemory;


static uint32_t getHandleIndex(Memory memory)
{
    uint32_t handleIndex = memory.handle.value & 0x00ff'ffffu;
    return handleIndex;
}
static uint32_t getHandleIteration(Memory memory)
{
    uint32_t iteration = (memory.handle.value >> 24u) & 0xffu;
    return iteration;
}

void initMemory()
{
    if(allMemory.inited)
        return;
    allMemory.memory = new uint8_t[AllocatedSize];
    uint8_t *newAlignedData =
        (uint8_t *)((((uintptr_t)allMemory.memory) + MemoryAlignment - 1u) & ~(((uintptr_t)MemoryAlignment) - 1u));

    allMemory.memoryStart = newAlignedData;
    allMemory.inited = true;

    {
        uint8_t *ptr = allMemory.memory;

        for(uint32_t ind = 0u; ind < AllocatedSize; ++ind)
        {
            *ptr++ = DebugValue;
        }
    }
    // Initialize freedallocations
    {
        for(uint32_t i = 0; i < MaxAllocations; ++i)
        {
            allMemory.freedAllocationIndices[MaxAllocations - i - 1] = i;
            allMemory.usedAllocationIndices[i] = ~0u;
        }
        allMemory.freedAllocationCount = MaxAllocations;
    }
    printf("Start: %p - aligned start: %p\n", allMemory.memory, newAlignedData);
}


Memory allocateMemoryBytes(uint32_t size)
{
    printf("allocating: %u -> ", size);
    if(size == 0)
        return Memory{ ~0u };
    size = (size + MinimumMemoryChunkSize - 1u) & (~(MinimumMemoryChunkSize - 1u));

    printf(" turned into allocating: %u\n", size);

    if(!allMemory.inited)
    {
        initMemory();
    }

    if(MaxMemorySize - allMemory.memoryPointer < size)
    {
        printf("All memory used: %u\n", allMemory.memoryPointer + size);
        ASSERT(MaxMemorySize - allMemory.memoryPointer >= size);
        exit(1);
    }
    uint32_t index = ~0u;
    if(allMemory.freedAllocationCount > 0)
    {
        index = allMemory.freedAllocationIndices[allMemory.freedAllocationCount - 1];
        allMemory.freedAllocationCount -= 1;
    }
    else
    {
        printf("Too many allocations: %u\n", allMemory.allocationCount);
        ASSERT(allMemory.allocationCount < MaxAllocations);
        exit(1);
    }
    if(index >= MaxAllocations)
    {
        printf("Too many allocations, index was out of allocationcount: %u\n", index);
        ASSERT(index < MaxAllocations);
        exit(1);
    }
    if(allMemory.usedAllocationIndices[allMemory.allocationCount] != ~0u)
    {
        printf("For some reason this index is already in use: %u\n", allMemory.allocationCount);
        ASSERT(allMemory.usedAllocationIndices[allMemory.allocationCount] == ~0u);
        exit(1);
    }
    MemoryArea &alloc = allMemory.memoryAreas[index];
    alloc.startIndex = allMemory.memoryPointer;
    alloc.size = size;
    allMemory.memoryPointer += size;

    printf("Memory: %u, size: %u\n", alloc.startIndex, alloc.size);

    allMemory.usedAllocationIndices[allMemory.allocationCount] = index;
    allMemory.allocationCount++;
    printf("adding allocations, allocations: %u\n", allMemory.allocationCount);

    allMemory.handleIterations[index] = (allMemory.handleIterations[index] + 1) & 0xffu;

    Memory result = Memory{ index | (allMemory.handleIterations[index] << 24u) };
    return result;
}


bool deAllocate(Memory memory)
{
    if(allMemory.allocationCount == 0)
        return false;
    if(!isValidMemory(memory))
        return false;

    uint32_t handleIndex = getHandleIndex(memory);

    // If the memory is last allocated memory, it should be last in memory, so it can be
    // just freed into stack.
    if(handleIndex == allMemory.usedAllocationIndices[allMemory.allocationCount - 1])
    {
        allMemory.usedAllocationIndices[allMemory.allocationCount - 1] = ~0u;

        MemoryArea &alloc = allMemory.memoryAreas[handleIndex];
        allMemory.memoryPointer -= alloc.size;
        alloc.startIndex = 0;
        alloc.size = 0;
    }
    else
    {
        uint32_t allocs = allMemory.allocationCount;
        bool found = false;
        uint32_t i = 0;
        for(;i < allocs; ++i)
        {
            if(handleIndex == allMemory.usedAllocationIndices[i])
            {
                found = true;
                for(; i < allocs - 1; ++i)
                {
                    allMemory.usedAllocationIndices[i] = allMemory.usedAllocationIndices[i + 1];
                }
                allMemory.usedAllocationIndices[i] = ~0u;
            }
        }
        if(!found)
            return false;
    }

    allMemory.handleIterations[handleIndex] = (allMemory.handleIterations[handleIndex] + 1) & 0xffu;

    allMemory.freedAllocationIndices[allMemory.freedAllocationCount] = handleIndex;
    allMemory.freedAllocationCount += 1;
    allMemory.allocationCount -= 1;
    printf("rmemoving allocations, allocations left: %u\n", allMemory.allocationCount);
    return true;
}

void defragMemory()
{
    printf("Defrag memory\n");

    uint32_t memoryStart = 0u;
    for(uint32_t i = 0; i < allMemory.allocationCount; ++i)
    {
        uint32_t index = allMemory.usedAllocationIndices[i];
        MemoryArea &area = allMemory.memoryAreas[index];
        ASSERT(memoryStart <= area.startIndex);
        if(memoryStart < area.startIndex)
        {
            memmove(allMemory.memoryStart + memoryStart, allMemory.memoryStart + area.startIndex, area.size);
        }
        memoryStart += area.size;
    }
    ASSERT(memoryStart < MaxMemorySize);
    allMemory.memoryPointer = memoryStart;
}

bool isValidMemory(Memory memory)
{
    uint32_t handleIndex = getHandleIndex(memory);
    uint32_t iteration = getHandleIteration(memory);
    if(handleIndex >= MaxAllocations)
        return false;
    return allMemory.handleIterations[handleIndex] == iteration;
}
