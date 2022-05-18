#include "bytebuffer.h"
#include <core/mytypes.h>

#include <string.h>

ByteBuffer::ByteBuffer(uint32_t dataTypeSize)
    : bufferData {
         .size = 0,
         .capasity = 0,
         .dataTypeSize = dataTypeSize,
         .memory = Memory{}
    }
{
    //printf("ds: %u, ", dataTypeSize);
}

ByteBuffer::~ByteBuffer()
{
    deAllocateMemory(bufferData.memory);
}

void ByteBuffer::clear()
{
    bufferData.size = 0;
}

void ByteBuffer::copyFrom(const ByteBuffer &other)
{
    bufferData.dataTypeSize = other.bufferData.dataTypeSize;
    reserve(other.bufferData.capasity);
    memmove(getBegin(), other.getBegin(), other.bufferData.size * other.bufferData.dataTypeSize);

    bufferData.size = other.bufferData.size;
}

void ByteBuffer::copyFromArray(uint8_t *arr, uint32_t count)
{
    reserve(count * bufferData.size);
    memmove(getBegin(), arr, count * bufferData.dataTypeSize);

    bufferData.size = count;
}

void ByteBuffer::reserve(uint32_t indices)
{
    bufferData.memory = resizeMemory(bufferData.memory, indices * bufferData.dataTypeSize);
    bufferData.capasity = indices;
}


void ByteBuffer::resize(uint32_t dstIndiceCount)
{
    if(bufferData.capasity < dstIndiceCount)
    {
        reserve(dstIndiceCount);
    }
    bufferData.size = dstIndiceCount;
}

void ByteBuffer::resize(uint32_t newSize, uint8_t *defaultValue)
{
    uint32_t oldSize = bufferData.size;
    resize(newSize);
    uint8_t *ptr = getBegin() + oldSize * bufferData.dataTypeSize;
    while(oldSize < newSize)
    {
        memmove(ptr, defaultValue, bufferData.dataTypeSize);
        ptr += bufferData.dataTypeSize;
        ++oldSize;
    }
}

void ByteBuffer::insertIndex(uint32_t index)
{
    if(bufferData.size + 1 >= bufferData.capasity)
    {
        uint32_t minCapasity = 256 / bufferData.dataTypeSize;
        minCapasity = minCapasity < 8 ? 8 : minCapasity;
        uint32_t newCapasity = bufferData.capasity < minCapasity ? minCapasity : bufferData.capasity * 2u;
        reserve(newCapasity);
        bufferData.capasity = newCapasity;
    }
    uint8_t *startPtr = getBegin() + index * bufferData.dataTypeSize;
    // move everything forward, if not adding to last
    if(index + 1 < bufferData.size)
    {
        memmove(startPtr + bufferData.dataTypeSize, startPtr,
            bufferData.dataTypeSize * (bufferData.size - index));
    }
    bufferData.size += 1;
}

void ByteBuffer::insertIndex(uint32_t index, const uint8_t *obj)
{
    insertIndex(index);
    uint8_t *startPtr = getBegin() + index * bufferData.dataTypeSize;
    memmove(startPtr, obj, bufferData.dataTypeSize);
}

void ByteBuffer::removeIndex(uint32_t index)
{
    ASSERT(index < bufferData.size);
    if (index >= bufferData.size)
        return;

    if (index < bufferData.size - 1)
    {
        uint8_t *myBegin = getMemoryBegin(bufferData.memory);
        memmove(getBegin() + index * bufferData.dataTypeSize,
            getBegin() + (index + 1) * bufferData.dataTypeSize,
            bufferData.dataTypeSize * (bufferData.size - (index + 1))
        );
    }
    --bufferData.size;
    return;
}

uint8_t *ByteBuffer::getDataIndex(uint32_t index) const
{
    ASSERT(index < bufferData.size);
    if (index >= bufferData.size)
        return nullptr;

    return getBegin() + index * bufferData.dataTypeSize;
}

uint8_t *ByteBuffer::getBegin() const
{
    return getMemoryBegin(bufferData.memory);
}
uint8_t *ByteBuffer::getEnd() const
{
    return getMemoryEnd(bufferData.memory);
}

