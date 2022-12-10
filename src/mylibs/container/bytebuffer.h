#pragma once

#include "mymemory.h"

#include <core/mytypes.h>

enum BufferType : u16
{
    PODVECTOR,
    VECTOR,
    STRING,
    UNKNOWN
};

struct ByteBufferData
{
    u32 size = 0;
    u32 capasity = 0;
    Memory memory {};
    u16 dataTypeSize = 0;
    BufferType dataBufferType = BufferType::UNKNOWN;
};

class ByteBuffer
{
public:

    ByteBuffer(u32 dataTypeSize, BufferType bufferType);
    ~ByteBuffer();
    // just sets size to 0
    void clear();
    // actually deallocate and return to initial state.
    void reset();
    void copyFrom(const ByteBuffer &other);
    void copyFromArray(u8 *arr, u32 count);
    void reserve(u32 indices);
    void resize(u32 dstIndiceCount);
    void resize(u32 newSize, u8 *defaultValue);
    void insertIndex(u32 index, const u8 *obj);
    void insertIndex(u32 index);

    void removeIndex(u32 index);
    u8 *getDataIndex(u32 index) const;
    u8 *getBegin() const;
    u8 *getEnd() const;

    u32 getSize() const { return bufferData.size; }
    u32 getCapasity() const { return bufferData.capasity; }
    u32 getDataSize() const { return bufferData.dataTypeSize; }

    BufferType getBufferType() const { return bufferData.dataBufferType; }

private:
    ByteBufferData bufferData {};
};



double getByteBufferTimer();
