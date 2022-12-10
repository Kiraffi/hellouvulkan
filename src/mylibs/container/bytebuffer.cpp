#include "bytebuffer.h"

#include <core/assert.h>
#include <core/general.h>
#include <core/mytypes.h>
#include <core/timer.h>

ByteBuffer::ByteBuffer(u32 dataTypeSize, BufferType bufferType)
    : bufferData {
         .size = 0,
         .capasity = 0,
         .memory = Memory{},
         .dataTypeSize = u16(dataTypeSize),
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

void ByteBuffer::copyFromArray(u8 *arr, u32 count)
{
    reserve(count * bufferData.size);
    Supa::memmove(getBegin(), arr, count * bufferData.dataTypeSize);

    bufferData.size = count;
}

void ByteBuffer::reserve(u32 indices)
{
    if(bufferData.capasity < indices)
    {
        bufferData.memory = resizeMemory(bufferData.memory, indices * bufferData.dataTypeSize);
        bufferData.capasity = indices;
    }
}


void ByteBuffer::resize(u32 dstIndiceCount)
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

void ByteBuffer::resize(u32 newSize, u8 *defaultValue)
{
    u32 oldSize = bufferData.size;
    resize(newSize);
    u8 *ptr = getBegin() + oldSize * bufferData.dataTypeSize;
    while(oldSize < newSize)
    {
        Supa::memmove(ptr, defaultValue, bufferData.dataTypeSize);
        ptr += bufferData.dataTypeSize;
        ++oldSize;
    }
}

void ByteBuffer::insertIndex(u32 index)
{
    if(bufferData.size + 1 >= bufferData.capasity)
    {
        u32 minCapasity = 256 / bufferData.dataTypeSize;
        minCapasity = minCapasity < 8 ? 8 : minCapasity;
        u32 newCapasity = bufferData.capasity < minCapasity ? minCapasity : bufferData.capasity * 2u;
        reserve(newCapasity);
        bufferData.capasity = newCapasity;
    }

    // move everything forward, if not adding to last
    if(index < bufferData.size)
    {
        u8 *startPtr = getBegin() + index * bufferData.dataTypeSize;
        Supa::memmove(startPtr + bufferData.dataTypeSize, startPtr,
            bufferData.dataTypeSize * (bufferData.size - index));
    }
    bufferData.size += 1;
}
static Timer bytebufferTimer;
void ByteBuffer::insertIndex(u32 index, const u8 *obj)
{
    //bytebufferTimer.continueTimer();
    insertIndex(index);
    u8 *startPtr = getBegin() + index * bufferData.dataTypeSize;
    Supa::memmove(startPtr, obj, bufferData.dataTypeSize);
    //bytebufferTimer.pauseTimer();
}

void ByteBuffer::removeIndex(u32 index)
{
    ASSERT(index < bufferData.size);
    if (index >= bufferData.size)
        return;

    if (index < bufferData.size - 1)
    {
        u8 *myBegin = getMemoryBegin(bufferData.memory);
        Supa::memmove(getBegin() + index * bufferData.dataTypeSize,
            getBegin() + (index + 1) * bufferData.dataTypeSize,
            bufferData.dataTypeSize * (bufferData.size - (index + 1))
        );
    }
    --bufferData.size;
    return;
}

u8 *ByteBuffer::getDataIndex(u32 index) const
{
    ASSERT(index < bufferData.size);
    if (index >= bufferData.size)
        return nullptr;
    u8 *beg = getBegin();
    return beg + index * bufferData.dataTypeSize;
}

u8 *ByteBuffer::getBegin() const
{
    return getMemoryBegin(bufferData.memory);
}

u8 *ByteBuffer::getEnd() const
{
    return getMemoryEnd(bufferData.memory);
}

double getByteBufferTimer()
{
    double v = bytebufferTimer.getDuration();
    bytebufferTimer.resetTimer();
    return v;

}
