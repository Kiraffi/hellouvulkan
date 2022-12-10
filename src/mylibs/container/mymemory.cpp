#include "mymemory.h"
#include <core/assert.h>

#include <core/general.h>
#include <core/mytypes.h>
#include <core/timer.h>

// memmove, exit
//#include <string.h>
// exit()
//#include <stdlib.h>

// What to initialize originally the memory, probably not really useful, since
// it would have to initialize that during freeing.
#define USE_DEBUGVALUE 0
static constexpr u8 DebugValue = 0xabu;

#define USE_PRINTING 0
#define PRINT_ALLOCATION_ADDRESS 0



#if PRINT_ALLOCATION_ADDRESS
#if _MSC_VER
    #include <intrin.h>
    #define returnAddress _AddressOfReturnAddress()
#else
    #define returnAddress __builtin_frame_address(0)
#endif
#endif


struct MemoryArea
{
    #if PRINT_ALLOCATION_ADDRESS
        void* ptrOfAddress = nullptr;
        u64 allocationNumber = ~u64(0);
    #endif // PRINT_ALLOCATION_ADDRESS
    u32 startLocation = 0;
    u32 size = 0;
};


///// THIS WILL NOT TO WORK WITH MULTIPLE THREADS!
static constexpr u32 MaxAllocations = 65536u;
static constexpr u32 MaxMemorySize = 64u * 1024u * 1024u;
static constexpr u32 AllocatedSize = MaxMemorySize + 65536u +
    (sizeof(MemoryArea) + sizeof(u32) * 3) * MaxAllocations;

// Also alignment!!!
static constexpr u32 MinimumMemoryChunkSize = 256u;
static constexpr u32 MemoryAlignment = 4096u;

struct AllMemory;
static AllMemory *allMemory = nullptr;

#if PRINT_ALLOCATION_ADDRESS
    static u64 AllocationNumber = 0;
#endif
struct AllMemory
{
    ~AllMemory()
    {
        printf("Deleting memory, max usage: %u!\n", maxUsed);
        if (allocationCount > 0)
        {
            printf("Allocations alive: %u\n", allocationCount);
            #if PRINT_ALLOCATION_ADDRESS
            for (i32 i = 0; i < allocationCount; ++i)
            {
                printf("Index: %i, Allocation pointer: %p, numb: %u\n",
                    i, memoryAreas[i].ptrOfAddress, memoryAreas[i].allocationNumber);
            }
            #endif //PRINT_ALLOCATION_ADDRESS
            ASSERT(false);
        }
        if(memoryAll)
            delete[] memoryAll;
        memoryAll = nullptr;
        memoryAligned = nullptr;
        allMemory = nullptr;
    }

    u8 *memoryAll = nullptr;
    u8 *memoryAligned = nullptr;

    MemoryArea *memoryAreas = nullptr; //[MaxAllocations] = {};
    u32 *freedAllocationIndices = nullptr; //[MaxAllocations] = {};
    u32 *usedAllocationIndices = nullptr; //[MaxAllocations] = {};

    u32 *handleIterations = nullptr; //[MaxAllocations] = {};


    //MemoryArea memoryAreas[MaxAllocations] = {};
    //u32 freedAllocationIndices[MaxAllocations] = {};
    //u32 usedAllocationIndices[MaxAllocations] = {};

    //u32 handleIterations[MaxAllocations] = {};


    u32 freedAllocationCount = 0;
    u32 allocationCount = 0;

    u32 memoryUsed = 0;
    u32 maxUsed = 0u;
    bool inited = false;
    bool needsDefrag = false;
};


static u32 getHandleIndex(Memory memory)
{
    u32 handleIndex = memory.handle.value & 0x00ff'ffffu;
    return handleIndex;
}
static u32 getHandleIteration(Memory memory)
{
    u32 iteration = (memory.handle.value >> 24u) & 0xffu;
    return iteration;
}


static void initMemoryReal()
{
    ASSERT(allMemory == nullptr);
    if(allMemory)
        return;
    allMemory = new AllMemory();

    uintptr_t startOffset = 0;
    allMemory->memoryAll = new u8[AllocatedSize];

    allMemory->memoryAreas = (MemoryArea *)allMemory->memoryAll;
    startOffset += MaxAllocations * sizeof(MemoryArea);

    allMemory->freedAllocationIndices = (u32 *)(allMemory->memoryAll + startOffset);
    startOffset += MaxAllocations * sizeof(u32);

    allMemory->usedAllocationIndices = (u32 *)(allMemory->memoryAll + startOffset);
    startOffset += MaxAllocations * sizeof(u32);

    allMemory->handleIterations = (u32 *)(allMemory->memoryAll + startOffset);
    startOffset += MaxAllocations * sizeof(u32);

    startOffset += MemoryAlignment;

    u8 *newAlignedData =
        (u8 *)((((uintptr_t)allMemory->memoryAll + startOffset) + MemoryAlignment - 1u) & ~(((uintptr_t)MemoryAlignment) - 1u));

    allMemory->memoryAligned = newAlignedData;
    allMemory->inited = true;

    #if USE_DEBUGVALUE
    {
        u8 *ptr = allMemory->memory;

        for(u32 ind = 0u; ind < AllocatedSize; ++ind)
        {
            *ptr++ = DebugValue;
        }
    }
    #endif
    // Initialize freedallocations
    {
        for(u32 i = 0; i < MaxAllocations; ++i)
        {
            allMemory->freedAllocationIndices[MaxAllocations - i - 1] = i;
            allMemory->usedAllocationIndices[i] = ~0u;
        }
        allMemory->freedAllocationCount = MaxAllocations;
    }
    #if USE_PRINTING
        printf("Start: %p - aligned start: %p\n", allMemory->memoryAligned, newAlignedData);
    #endif
}

void initMemory()
{
    if(allMemory)
        printf("Allocations at init memory: %u\n", allMemory->allocationCount);
    ASSERT(allMemory == nullptr);
    initMemoryReal();
}

Memory allocateMemoryBytes(u32 size)
{
    #if PRINT_ALLOCATION_ADDRESS
        void* pvAddressOfReturnAddress = returnAddress;
        printf("Allocated %p, numb: %u\n", pvAddressOfReturnAddress, AllocationNumber);
    #endif // PRINT_ALLOCATION_ADDRESS
    #if USE_PRINTING
        printf("allocating: %u -> ", size);
    #endif

    if(size == 0)
        return Memory{ ~0u };
    size = (size + MinimumMemoryChunkSize - 1u) & (~(MinimumMemoryChunkSize - 1u));

    #if USE_PRINTING
        printf(" turned into allocating: %u\n", size);
    #endif


    if(!allMemory->inited)
    {
        initMemoryReal();
    }

    if(MaxMemorySize - allMemory->memoryUsed < size)
    {
        printf("All memory used: %u\n", allMemory->memoryUsed + size);
        ASSERT(MaxMemorySize - allMemory->memoryUsed >= size);
        //exit(1);
    }
    u32 index = ~0u;
    if(allMemory->freedAllocationCount > 0)
    {
        index = allMemory->freedAllocationIndices[allMemory->freedAllocationCount - 1];
        allMemory->freedAllocationCount -= 1;
    }
    else
    {
        printf("Too many allocations: %u\n", allMemory->allocationCount);
        ASSERT(allMemory->allocationCount < MaxAllocations);
        //exit(1);
    }
    if(index >= MaxAllocations)
    {
        printf("Too many allocations, index was out of allocationcount: %u\n", index);
        ASSERT(index < MaxAllocations);
        //exit(1);
    }
    if(allMemory->usedAllocationIndices[allMemory->allocationCount] != ~0u)
    {
        printf("For some reason this index is already in use: %u\n", allMemory->allocationCount);
        ASSERT(allMemory->usedAllocationIndices[allMemory->allocationCount] == ~0u);
        //exit(1);
    }
    MemoryArea &alloc = allMemory->memoryAreas[index];
    ASSERT((allMemory->memoryUsed % MinimumMemoryChunkSize) == 0);
    alloc.startLocation = allMemory->memoryUsed;
    alloc.size = size;

    #if PRINT_ALLOCATION_ADDRESS
        alloc.ptrOfAddress = pvAddressOfReturnAddress;
        alloc.allocationNumber = AllocationNumber++;
        ASSERT(alloc.ptrOfAddress);
    #endif // PRINT_ALLOCATION_ADDRESS

    allMemory->memoryUsed += size;
    //printf("Size: %u, total: %u\n", size, allMemory->memoryUsed);
    if(allMemory->maxUsed < allMemory->memoryUsed)
        allMemory->maxUsed = allMemory->memoryUsed;


    allMemory->usedAllocationIndices[allMemory->allocationCount] = index;
    allMemory->allocationCount++;

    #if USE_PRINTING
        printf("adding allocations, allocations: %u\n", allMemory->allocationCount);
        printf("Memory: %u, size: %u\n", alloc.startLocation, alloc.size);
    #endif

    allMemory->handleIterations[index] = (allMemory->handleIterations[index] + 1) & 0xffu;

    Memory result = Memory{ index | (allMemory->handleIterations[index] << 24u) };
    return result;
}

bool deAllocateMemory(Memory memory)
{
    if(!allMemory || allMemory->allocationCount == 0)
        return false;
    if(!isValidMemory(memory))
        return false;

    u32 handleIndex = getHandleIndex(memory);

    // If the memory is last allocated memory, it should be last in memory, so it can be
    // just freed into stack.
    if(handleIndex == allMemory->usedAllocationIndices[allMemory->allocationCount - 1])
    {
        MemoryArea &alloc = allMemory->memoryAreas[handleIndex];
        allMemory->memoryUsed -= alloc.size;

        allMemory->usedAllocationIndices[allMemory->allocationCount - 1] = ~0u;
    }
    else
    {
        u32 allocs = allMemory->allocationCount;
        bool found = false;
        u32 i = 0;
        for(;i < allocs; ++i)
        {
            if(handleIndex == allMemory->usedAllocationIndices[i])
            {
                found = true;
                for(; i < allocs - 1; ++i)
                {
                    allMemory->usedAllocationIndices[i] = allMemory->usedAllocationIndices[i + 1];
                }
                allMemory->usedAllocationIndices[i] = ~0u;
                allMemory->needsDefrag = true;
            }
        }
        if(!found)
            return false;
    }

    MemoryArea &alloc = allMemory->memoryAreas[handleIndex];
    alloc.startLocation = 0;
    alloc.size = 0;

    #if PRINT_ALLOCATION_ADDRESS
        printf("deleted: %p, numb: %u\n", alloc.ptrOfAddress, alloc.allocationNumber);
        //alloc.ptrOfAddress = nullptr;
    #endif // PRINT_ALLOCATION_ADDRESS
    allMemory->handleIterations[handleIndex] = (allMemory->handleIterations[handleIndex] + 1) & 0xffu;

    allMemory->freedAllocationIndices[allMemory->freedAllocationCount] = handleIndex;
    allMemory->freedAllocationCount += 1;
    allMemory->allocationCount -= 1;

    #if USE_PRINTING
        printf("removing allocation, allocations left: %u\n", allMemory->allocationCount);
    #endif

    return true;
}

Memory resizeMemory(Memory memory, u32 size)
{
    #if USE_PRINTING
        printf("Reallocating!\n");
    #endif
    size = (size + MinimumMemoryChunkSize - 1u) & (~(MinimumMemoryChunkSize - 1u));

    ASSERT(size < MaxMemorySize);
    if(size >= MaxMemorySize)
    {
        printf("Trying to allocate too much memory!");
        //exit(1);
    }

    if(!isValidMemory(memory))
    {
        return allocateMemoryBytes(size);
    }
    u32 handleIndex = getHandleIndex(memory);
    MemoryArea &oldArea = allMemory->memoryAreas[handleIndex];
    if(oldArea.size > size)
    {
        return memory;
    }
    bool lastIndex = allMemory->usedAllocationIndices[allMemory->allocationCount - 1] == handleIndex;
    if(lastIndex)
    {
        allMemory->memoryUsed += size - oldArea.size;
        ASSERT(allMemory->memoryUsed <= MaxMemorySize);
        if(allMemory->memoryUsed > MaxMemorySize)
        {
            printf("Trying to allocate too much memory!");
            ASSERT(false);
            //exit(1);
        }
        oldArea.size = size;
    }
    else
    {
        Memory newMemory = allocateMemoryBytes(size);
        if(!isValidMemory(newMemory))
        {
            printf("Trying to allocate too much memory!");
            ASSERT(false);
            //exit(1);
        }
        u32 newHandle = getHandleIndex(newMemory);
        MemoryArea &newArea = allMemory->memoryAreas[newHandle];
        Supa::memmove(allMemory->memoryAligned + newArea.startLocation,
            allMemory->memoryAligned + oldArea.startLocation, oldArea.size);

        deAllocateMemory(memory);
        memory = newMemory;
    }
    return memory;
}


void defragMemory()
{
    if(!allMemory->needsDefrag)
        return;
    ScopedTimer defragDuration("Defrag duration");
    allMemory->needsDefrag = false;

    #if USE_PRINTING
        printf("Defrag memory\n");
    #endif

    u32 memoryCount = 0u;
    for(u32 i = 0; i < allMemory->allocationCount; ++i)
    {
        u32 index = allMemory->usedAllocationIndices[i];
        MemoryArea &area = allMemory->memoryAreas[index];
        ASSERT(memoryCount <= area.startLocation);
        if(memoryCount < area.startLocation)
        {
            Supa::memmove(allMemory->memoryAligned + memoryCount, allMemory->memoryAligned + area.startLocation, area.size);
            area.startLocation = memoryCount;
        }
        memoryCount += area.size;
    }
    ASSERT(memoryCount < MaxMemorySize);

    #if USE_DEBUGVALUE
    {
        if(memoryCount < allMemory->memoryUsed)
            memset(allMemory->memoryAligned + memoryCount, DebugValue, allMemory->memoryUsed - memoryCount);
    }
    #endif

    allMemory->memoryUsed = memoryCount;
}

bool isValidMemory(Memory memory)
{
    if (!allMemory || allMemory->memoryAll == nullptr || allMemory->memoryAligned == nullptr)
        return false;
    u32 handleIndex = getHandleIndex(memory);
    u32 iteration = getHandleIteration(memory);
    if(handleIndex >= MaxAllocations)
        return false;
    return allMemory->handleIterations[handleIndex] == iteration;
}

u8 *getMemoryBegin(Memory memory)
{
    bool validMemory = isValidMemory(memory);
    if(!validMemory)
        return nullptr;
    ASSERT(validMemory);
    u32 handleIndex = getHandleIndex(memory);
    bool isValidHandle = handleIndex < MaxAllocations;
    ASSERT(isValidHandle);
    if(!validMemory || !isValidHandle)
        return nullptr;
    const MemoryArea &area = allMemory->memoryAreas[handleIndex];
    return allMemory->memoryAligned + area.startLocation;
}

u8 *getMemoryEnd(Memory memory)
{
    bool validMemory = isValidMemory(memory);
    if(!validMemory)
        return nullptr;
    ASSERT(validMemory);
    u32 handleIndex = getHandleIndex(memory);
    bool isValidHandle = handleIndex < MaxAllocations;
    ASSERT(isValidHandle);
    if(!validMemory || !isValidHandle)
        return nullptr;
    const MemoryArea &area = allMemory->memoryAreas[handleIndex];
    return allMemory->memoryAligned + area.startLocation + area.size;
}

void deinitMemory()
{
    delete allMemory;
    allMemory = nullptr;
}