#pragma once

#include "bytebuffer.h"
#include <memory>

template <typename T>
class PodVector final
{
public:
    PodVector() : buffer(sizeof(T))
    {
        static constexpr bool layout = std::is_standard_layout<T>();
        //static constexpr bool trivial = std::is_trivial<T>();
        static constexpr bool trivialCopy = std::is_trivially_copyable<T>();
        static_assert(layout && trivialCopy, "Not pod class!");
    }
    ~PodVector();

    PodVector(const PodVector<T> &vec);
    PodVector(PodVector<T> &&other);
    PodVector& operator=(const PodVector<T> &vec);

    T &operator[] (uint32_t index) const;

    void pushBack(const T &obj);
    void insertIndex(uint32_t index, const T &obj);
    void removeIndex(uint32_t index);

    void clear();
    void resize(uint32_t newSize);
    void resize(uint32_t newSize, const T &defaultValue);

    void reserve(uint32_t newSize);
    T* begin();
    T* end();
    uint32_t getSize() const;
    uint32_t getCapasity() const;

private:
    ByteBuffer buffer;
};



template <typename T>
PodVector<T>::~PodVector()
{
    buffer.~ByteBuffer();
}

template <typename T>
T* PodVector<T>::begin()
{
    return (T*)buffer.getBegin();
}

template <typename T>
T* PodVector<T>::end()
{
    uint8_t *ptr = buffer.getBegin();
    uint8_t *end = buffer.getEnd();
    uint32_t bytes = sizeof(T) * buffer.getSize();
    ptr += bytes;
    if(ptr <= end)
        return (T*)(ptr);
    return (T*)(end);
}


template <typename T>
PodVector<T>::PodVector(const PodVector<T> &vec)
{
    *this = vec;
}

template <typename T>
PodVector<T>::PodVector(PodVector &&other)
{
    new (&buffer) ByteBuffer(sizeof(T));
    memcpy(buffer, other.buffer, sizeof(buffer));

    new (&other.buffer) ByteBuffer(sizeof(T));
}

template <typename T>
PodVector<T>& PodVector<T>::operator=(const PodVector<T> &vec)
{
    if (&vec == this)
        return *this;

    new (&buffer) ByteBuffer(sizeof(T));

    buffer.copyFrom(vec.buffer);
    return *this;
}

template <typename T>
T & PodVector<T>::operator[] (uint32_t index) const
{
    uint8_t *ptr = buffer.getDataIndex(index);
    return (T &)(*ptr);
}

template <typename T>
void PodVector<T>::clear()
{
    buffer.clear();
}

template <typename T>
void PodVector<T>::resize(uint32_t newSize)
{
    buffer.resize(newSize);
}

template <typename T>
void PodVector<T>::resize(uint32_t newSize, const T &defaultValue)
{
    buffer.resize(newSize, (uint8_t *)&defaultValue);
}



template <typename T>
void PodVector<T>::reserve(uint32_t newSize)
{
    buffer.reserve(newSize);
}



template <typename T>
void PodVector<T>::pushBack(const T &obj)
{
    buffer.insertIndex(buffer.getSize(), (uint8_t *)&obj);
}

template <typename T>
void PodVector<T>::insertIndex(uint32_t index, const T &obj)
{
    buffer.insertIndex(index, (const uint8_t *)&obj);
}

template <typename T>
void PodVector<T>::removeIndex(uint32_t index)
{
    buffer.removeIndex(index);
}

template <typename T>
uint32_t PodVector<T>::getSize() const
{
    return buffer.getSize();
}

template <typename T>
uint32_t PodVector<T>::getCapasity() const
{
    return buffer.getCapasity();
}

