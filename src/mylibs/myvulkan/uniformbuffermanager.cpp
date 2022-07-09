#include "uniformbuffermanager.h"

#include <container/podvector.h>

void UniformBufferManager::init(Buffer &buf)
{ 
    buffer = &buf;
    usedIndices.resize(1024 / (sizeof(MemoryIndexType)), 0);
    inited = true;
}


void UniformBufferManager::freeHandle(UniformBufferHandle handle)
{
    ASSERT(inited);
    ASSERT(this == handle.manager);
    if (this != handle.manager)
        return;

    uint32_t index = handle.index - 1;
    uint32_t slot = index / (sizeof(MemoryIndexType) * 8);
    MemoryIndexType bit = 
        (MemoryIndexType(1) << MemoryIndexType(index % (sizeof(MemoryIndexType) * 8)));
    
    if((usedIndices[slot] & bit) == 0)
    {
        // notice the index is off by one.
        printf("Seems that the slot is already freed: %u\n", index);
        ASSERT((usedIndices[slot] & bit) != 0);
        return;
    }
    usedIndices[slot] &= ~bit;

    //printf("removing uniformbufferslot: %u\n", handle.index);
}

UniformBufferHandle UniformBufferManager::reserveHandle()
{
    ASSERT(inited);
    UniformBufferHandle result;
    uint32_t slot = 0u;
    static const MemoryIndexType MaxSize = ~(MemoryIndexType(0));

    // Find place that has at least one bit free.
    while (slot < usedIndices.size() && usedIndices[slot] == MaxSize)
        ++slot;

    ASSERT(slot < usedIndices.size());
    if(slot >= usedIndices.size())
        return result;

    MemoryIndexType mem = usedIndices[slot];

    // Count the first free index.
    result.index = slot * 64 + 1u;
    MemoryIndexType index = 1;
    while ((mem & index) != 0)
    {
        ++result.index;
        index = index << (MemoryIndexType(1));
    }
    usedIndices[slot] |= index;
    
    result.manager = this;
    result.size = 65536u;
    //printf("reserving uniform buffer slot: %u\n", result.index);
    return result;
};