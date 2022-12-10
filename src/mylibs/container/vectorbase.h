#pragma once

#include "bytebuffer.h"

#include <core/assert.h>
#include <core/mytypes.h>

class VectorBase
{
public:
    VectorBase(u32 size, BufferType bufferType) : buffer(size, bufferType) {}
    virtual ~VectorBase() { buffer.~ByteBuffer(); }

    void reserve(u32 newSize) { buffer.reserve(newSize); }

    u32 size() const { return buffer.getSize(); }
    u32 getSize() const { return size(); }
    u32 getCapasity() const { return buffer.getCapasity(); }
    u32 getDataSize() const { return buffer.getDataSize(); }
    bool empty() const { return size() == 0u; }

    ByteBuffer &getBuffer() { return buffer; }
    const ByteBuffer &getBuffer() const { return buffer; }

protected:
    void doClear() { buffer.clear(); }
    u8* getBegin() const { return buffer.getBegin(); }
    u8* getEnd() const
    {
        u8 *ptr = buffer.getBegin();
        u32 bytes = buffer.getDataSize() * buffer.getSize();
        ptr += bytes;
        return ptr;
    }

    u8* getBack() const
    {
        ASSERT(buffer.getSize() > 0);
        u8 *ptr = buffer.getBegin();
        ptr += buffer.getDataSize() * (buffer.getSize() - 1);
        return ptr;
    }


    ByteBuffer buffer;
};
