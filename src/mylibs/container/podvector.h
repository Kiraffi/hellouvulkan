#pragma once

#include "vectorbase.h"
#include "podvectorsbase.h"
#include "podvectortype.h"

#include <core/podtype.h>
#include <core/supa.h>

#include <initializer_list>
//#include <type_traits>
#include "podvectortypedefine.h"

#define CHECK_POD_MACRO() isPodType<T>();

template <typename T>
PodVector<T>::PodVector() : VectorBase(sizeof(T), BufferType::PODVECTOR)
{
    CHECK_POD_MACRO();
}

/*
template <typename T>
PodVector<T>::PodVector(u32 size) : VectorBase(sizeof(T), BufferType::PODVECTOR)
{
    CHECK_POD_MACRO();
    const T t{};
    this->buffer.resize(size, (u8 *)&t);
}
*/

template <typename T>
PodVector<T>::PodVector(const T *b, const T* e) : VectorBase(sizeof(T), BufferType::PODVECTOR)
{
    CHECK_POD_MACRO();
    ASSERT(b < e);
    i32 reserveSize = (intptr_t(e) - intptr_t(b)) / sizeof(T);
    ASSERT(reserveSize > 0);

    this->buffer.resize(reserveSize);
    Supa::memcpy(this->buffer.getDataIndex(0), b, reserveSize * sizeof(T));
}

template <typename T>
PodVector<T>::PodVector(const PodVector<T> &vec) : VectorBase(sizeof(T), BufferType::PODVECTOR)
{
    CHECK_POD_MACRO();
    // operator=-function
    *this = vec;
}

template <typename T>
PodVector<T>::PodVector(PodVector &&other) noexcept : VectorBase(sizeof(T), BufferType::PODVECTOR)
{
    CHECK_POD_MACRO();
    buffer.reset();
    buffer = other.buffer;
    other.buffer = ByteBuffer(sizeof(T), BufferType::PODVECTOR);
}

template <typename T>
PodVector<T>::PodVector(const std::initializer_list<T> &initializerList) :
    VectorBase(sizeof(T), BufferType::PODVECTOR)
{
    CHECK_POD_MACRO();
    u32 size = initializerList.size();
    this->buffer.resize(size);

    u32 counter = 0;
    u8* ptr = this->buffer.getDataIndex(0);
    Supa::memcpy(ptr, &*initializerList.begin(), size * sizeof(T));
}



template <typename T>
PodVector<T>& PodVector<T>::operator=(const PodVector<T> &vec)
{
    CHECK_POD_MACRO();

    if (&vec == this)
        return *this;
    clear();
    this->buffer.copyFrom(vec.buffer);
    return *this;
}



template <typename T>
void PodVector<T>::resize(u32 newSize)
{
    resize(newSize, T());
}

template <typename T>
void PodVector<T>::resize(u32 newSize, const T &defaultValue)
{
    this->buffer.resize(newSize, (u8 *)&defaultValue);
}

template <typename T>
void PodVector<T>::uninitializedResize(u32 newSize)
{
    this->buffer.resize(newSize);
}

template <typename T>
T PodVector<T>::popBack()
{
    T t = back();
    buffer.removeIndex(buffer.getSize() - 1);
    return t;
}


template <typename T>
u32 PodVector<T>::find(const T &value) const
{
    u32 result = 0u;
    for(const auto &v : *this)
    {
        if(value == v)
            return result;
        ++result;
    }
    return ~0u;
}




template <typename T>
void PodVector<T>::pushBack(const T &obj)
{
    this->buffer.insertIndex(this->buffer.getSize(), (u8 *)&obj);
}


template <typename T>
void PodVector<T>::pushBack(const PodVector<T> &vec)
{
    if (vec.size() == 0)
        return;
    u32 oldSize = size();
    this->buffer.resize(size() + vec.size());
    Supa::memcpy(this->buffer.getDataIndex(oldSize), vec.data(), vec.size() * sizeof(T));
}

template <typename T>
void PodVector<T>::insertIndex(u32 index, const T &obj)
{
    this->buffer.insertIndex(index, (const u8 *)&obj);
}

template <typename T>
void PodVector<T>::removeIndex(u32 index)
{
    this->buffer.removeIndex(index);
}

