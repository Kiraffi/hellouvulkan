#pragma once

#include "bytebuffer.h"

#include <core/assert.h>
#include <core/mytypes.h>

#include <memory>
#include <string.h>

class VectorBase
{
public:
    VectorBase(uint32_t size) : buffer(size) {}
    virtual ~VectorBase() { buffer.~ByteBuffer(); }

    void reserve(uint32_t newSize) { buffer.reserve(newSize); }

    uint32_t size() const { return buffer.getSize(); }
    uint32_t getSize() const { return size(); }
    uint32_t getCapasity() const { return buffer.getCapasity(); }
    uint32_t getDataSize() const { return buffer.getDataSize(); }
    bool empty() const { return size() == 0u; }

protected:
    void doClear() { buffer.clear(); }
    uint8_t* getBegin() const { return buffer.getBegin(); }
    uint8_t* getEnd() const
    {
        uint8_t *ptr = buffer.getBegin();
        uint32_t bytes = buffer.getDataSize() * buffer.getSize();
        ptr += bytes;
        return ptr;
    }

    uint8_t* getBack() const
    {
        ASSERT(buffer.getSize() > 0);
        uint8_t *ptr = buffer.getBegin();
        ptr += buffer.getDataSize() * (buffer.getSize() - 1);
        return ptr;
    }


    ByteBuffer buffer;
};
