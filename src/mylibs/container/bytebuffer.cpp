#include "bytebuffer.h"

#include <core/assert.h>
#include <core/general.h>
#include <core/mytypes.h>
#include <core/timer.h>

ByteBuffer::ByteBuffer(uint32_t dataTypeSize, BufferType bufferType)
    : bufferData {
         .size = 0,
         .capasity = 0,
         .memory = Memory{},
         .dataTypeSize = uint16_t(dataTypeSize),
         .dataBufferType = bufferType,
    }
{
    ASSERT(dataTypeSize);
    ASSERT(dataTypeSize < 65536u);
    //printf("ds: %u, ", dataTypeSize);
}

ByteBuffer::~ByteBuffer()
{
    reset();
}
void ByteBuffer::reset()
{
    ASSERT(bufferData.dataTypeSize);
    if (isValidMemory(bufferData.memory) || bufferData.capasity > 0u || bufferData.size > 0u)
        ASSERT(deAllocateMemory(bufferData.memory));
    bufferData.capasity = 0u;
    bufferData.size = 0u;
    bufferData.memory = Memory{};
}

void ByteBuffer::clear()
{
    bufferData.size = 0;
}

void ByteBuffer::copyFrom(const ByteBuffer &other)
{
    bufferData.dataTypeSize = other.bufferData.dataTypeSize;
    reserve(other.bufferData.capasity);
    Supa::memmove(getBegin(), other.getBegin(), other.bufferData.size * other.bufferData.dataTypeSize);

    bufferData.size = other.bufferData.size;
}

void ByteBuffer::copyFromArray(uint8_t *arr, uint32_t count)
{
    reserve(count * bufferData.size);
    Supa::memmove(getBegin(), arr, count * bufferData.dataTypeSize);

    bufferData.size = count;
}

void ByteBuffer::reserve(uint32_t indices)
{
    if(bufferData.capasity < indices)
    {
        bufferData.memory = resizeMemory(bufferData.memory, indices * bufferData.dataTypeSize);
        bufferData.capasity = indices;
    }
}


void ByteBuffer::resize(uint32_t dstIndiceCount)
{

    if(bufferData.capasity < dstIndiceCount)
    {
        if(bufferData.capasity * 2 > dstIndiceCount)
            reserve(bufferData.capasity * 2);
        else
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
        Supa::memmove(ptr, defaultValue, bufferData.dataTypeSize);
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

    // move everything forward, if not adding to last
    if(index < bufferData.size)
    {
        uint8_t *startPtr = getBegin() + index * bufferData.dataTypeSize;
        Supa::memmove(startPtr + bufferData.dataTypeSize, startPtr,
            bufferData.dataTypeSize * (bufferData.size - index));
    }
    bufferData.size += 1;
}
static Timer bytebufferTimer;
void ByteBuffer::insertIndex(uint32_t index, const uint8_t *obj)
{
    //bytebufferTimer.continueTimer();
    insertIndex(index);
    uint8_t *startPtr = getBegin() + index * bufferData.dataTypeSize;
    Supa::memmove(startPtr, obj, bufferData.dataTypeSize);
    //bytebufferTimer.pauseTimer();
}

void ByteBuffer::removeIndex(uint32_t index)
{
    ASSERT(index < bufferData.size);
    if (index >= bufferData.size)
        return;

    if (index < bufferData.size - 1)
    {
        uint8_t *myBegin = getMemoryBegin(bufferData.memory);
        Supa::memmove(getBegin() + index * bufferData.dataTypeSize,
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
    uint8_t *beg = getBegin();
    return beg + index * bufferData.dataTypeSize;
}

uint8_t *ByteBuffer::getBegin() const
{
    return getMemoryBegin(bufferData.memory);
}

uint8_t *ByteBuffer::getEnd() const
{
    return getMemoryEnd(bufferData.memory);
}

double getByteBufferTimer()
{
    double v = bytebufferTimer.getDuration();
    bytebufferTimer.resetTimer();
    return v;

}
