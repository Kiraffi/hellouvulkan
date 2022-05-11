#include "uniformbuffermanager.h"

void UniformBufferManager::init(Buffer &buf)
{ 
    buffer = &buf;
    usedIndices.resize(64 * 1024 * 1024 / 8, 0);
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
    
    if(usedIndices[slot] & bit)
    {
        ASSERT(usedIndices[slot] & bit == 0);
        return;
    }

    usedIndices[slot] &= ~bit;
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
    
    return result;
};