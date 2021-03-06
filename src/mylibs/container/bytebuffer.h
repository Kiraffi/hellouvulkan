#pragma once

#include "mymemory.h"

enum BufferType : uint16_t
{
    PODVECTOR,
    VECTOR,
    STRING,
    UNKNOWN
};

struct ByteBufferData
{
    uint32_t size = 0;
    uint32_t capasity = 0;
    Memory memory {};
    uint16_t dataTypeSize = 0;
    BufferType dataBufferType = BufferType::UNKNOWN;
};

class ByteBuffer
{
public:

    ByteBuffer(uint32_t dataTypeSize, BufferType bufferType);
    ~ByteBuffer();
    // just sets size to 0
    void clear();
    // actually deallocate and return to initial state.
    void reset();
    void copyFrom(const ByteBuffer &other);
    void copyFromArray(uint8_t *arr, uint32_t count);
    void reserve(uint32_t indices);
    void resize(uint32_t dstIndiceCount);
    void resize(uint32_t newSize, uint8_t *defaultValue);
    void insertIndex(uint32_t index, const uint8_t *obj);
    void insertIndex(uint32_t index);

    void removeIndex(uint32_t index);
    uint8_t *getDataIndex(uint32_t index) const;
    uint8_t *getBegin() const;
    uint8_t *getEnd() const;

    uint32_t getSize() const { return bufferData.size; }
    uint32_t getCapasity() const { return bufferData.capasity; }
    uint32_t getDataSize() const { return bufferData.dataTypeSize; }

    BufferType getBufferType() const { return bufferData.dataBufferType; }

private:
    ByteBufferData bufferData {};
};



double getByteBufferTimer();
