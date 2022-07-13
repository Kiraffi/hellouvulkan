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
static constexpr uint8_t DebugValue = 0xabu;

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
        uint64_t allocationNumber = ~uint64_t(0);
    #endif // PRINT_ALLOCATION_ADDRESS
    uint32_t startLocation = 0;
    uint32_t size = 0;
};


///// THIS WILL NOT TO WORK WITH MULTIPLE THREADS!
static constexpr uint32_t MaxAllocations = 65536u;
static constexpr uint32_t MaxMemorySize = 64u * 1024u * 1024u;
static constexpr uint32_t AllocatedSize = MaxMemorySize + 65536u +
    (sizeof(MemoryArea) + sizeof(uint32_t) * 3) * MaxAllocations;

// Also alignment!!!
static constexpr uint32_t MinimumMemoryChunkSize = 256u;
static constexpr uint32_t MemoryAlignment = 4096u;

struct AllMemory;
static AllMemory *allMemory = nullptr;

#if PRINT_ALLOCATION_ADDRESS
    static uint64_t AllocationNumber = 0;
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
            for (int i = 0; i < allocationCount; ++i)
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

    uint8_t *memoryAll = nullptr;
    uint8_t *memoryAligned = nullptr;

    MemoryArea *memoryAreas = nullptr; //[MaxAllocations] = {};
    uint32_t *freedAllocationIndices = nullptr; //[MaxAllocations] = {};
    uint32_t *usedAllocationIndices = nullptr; //[MaxAllocations] = {};

    uint32_t *handleIterations = nullptr; //[MaxAllocations] = {};


    //MemoryArea memoryAreas[MaxAllocations] = {};
    //uint32_t freedAllocationIndices[MaxAllocations] = {};
    //uint32_t usedAllocationIndices[MaxAllocations] = {};

    //uint32_t handleIterations[MaxAllocations] = {};


    uint32_t freedAllocationCount = 0;
    uint32_t allocationCount = 0;

    uint32_t memoryUsed = 0;
    uint32_t maxUsed = 0u;
    bool inited = false;
    bool needsDefrag = false;
};


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


static void initMemoryReal()
{
    ASSERT(allMemory == nullptr);
    if(allMemory)
        return;
    allMemory = new AllMemory();

    uintptr_t startOffset = 0;
    allMemory->memoryAll = new uint8_t[AllocatedSize];

    allMemory->memoryAreas = (MemoryArea *)allMemory->memoryAll;
    startOffset += MaxAllocations * sizeof(MemoryArea);

    allMemory->freedAllocationIndices = (uint32_t *)(allMemory->memoryAll + startOffset);
    startOffset += MaxAllocations * sizeof(uint32_t);

    allMemory->usedAllocationIndices = (uint32_t *)(allMemory->memoryAll + startOffset);
    startOffset += MaxAllocations * sizeof(uint32_t);

    allMemory->handleIterations = (uint32_t *)(allMemory->memoryAll + startOffset);
    startOffset += MaxAllocations * sizeof(uint32_t);

    startOffset += MemoryAlignment;

    uint8_t *newAlignedData =
        (uint8_t *)((((uintptr_t)allMemory->memoryAll + startOffset) + MemoryAlignment - 1u) & ~(((uintptr_t)MemoryAlignment) - 1u));

    allMemory->memoryAligned = newAlignedData;
    allMemory->inited = true;

    #if USE_DEBUGVALUE
    {
        uint8_t *ptr = allMemory->memory;

        for(uint32_t ind = 0u; ind < AllocatedSize; ++ind)
        {
            *ptr++ = DebugValue;
        }
    }
    #endif
    // Initialize freedallocations
    {
        for(uint32_t i = 0; i < MaxAllocations; ++i)
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

Memory allocateMemoryBytes(uint32_t size)
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
    uint32_t index = ~0u;
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

    uint32_t handleIndex = getHandleIndex(memory);

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
        uint32_t allocs = allMemory->allocationCount;
        bool found = false;
        uint32_t i = 0;
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

Memory resizeMemory(Memory memory, uint32_t size)
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
    uint32_t handleIndex = getHandleIndex(memory);
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
        uint32_t newHandle = getHandleIndex(newMemory);
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

    uint32_t memoryCount = 0u;
    for(uint32_t i = 0; i < allMemory->allocationCount; ++i)
    {
        uint32_t index = allMemory->usedAllocationIndices[i];
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
    uint32_t handleIndex = getHandleIndex(memory);
    uint32_t iteration = getHandleIteration(memory);
    if(handleIndex >= MaxAllocations)
        return false;
    return allMemory->handleIterations[handleIndex] == iteration;
}

uint8_t *getMemoryBegin(Memory memory)
{
    bool validMemory = isValidMemory(memory);
    if(!validMemory)
        return nullptr;
    ASSERT(validMemory);
    uint32_t handleIndex = getHandleIndex(memory);
    bool isValidHandle = handleIndex < MaxAllocations;
    ASSERT(isValidHandle);
    if(!validMemory || !isValidHandle)
        return nullptr;
    const MemoryArea &area = allMemory->memoryAreas[handleIndex];
    return allMemory->memoryAligned + area.startLocation;
}

uint8_t *getMemoryEnd(Memory memory)
{
    bool validMemory = isValidMemory(memory);
    if(!validMemory)
        return nullptr;
    ASSERT(validMemory);
    uint32_t handleIndex = getHandleIndex(memory);
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