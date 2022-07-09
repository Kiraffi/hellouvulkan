#pragma once

#include "vectorbase.h"
#include "podvectorsbase.h"
#include <core/supa.h>

#include <initializer_list>
#include <type_traits>

#define CHECK_POD_MACRO() \
    static constexpr bool layout = std::is_standard_layout<T>(); \
    /*static constexpr bool trivial = std::is_trivial<T>();*/ \
    static constexpr bool trivialCopy = std::is_trivially_copyable<T>(); \
    static_assert(layout && trivialCopy, "Not pod class!");

template <typename T>
PodVector<T>::PodVector() : VectorBase(sizeof(T), BufferType::PODVECTOR)
{
    CHECK_POD_MACRO();
}

/*
template <typename T>
PodVector<T>::PodVector(uint32_t size) : VectorBase(sizeof(T), BufferType::PODVECTOR)
{
    CHECK_POD_MACRO();
    const T t{};
    this->buffer.resize(size, (uint8_t *)&t);
}
*/

template <typename T>
PodVector<T>::PodVector(const T *b, const T* e) : VectorBase(sizeof(T), BufferType::PODVECTOR)
{
    CHECK_POD_MACRO();
    ASSERT(b < e);
    int32_t reserveSize = (intptr_t(e) - intptr_t(b)) / sizeof(T);
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
    uint32_t size = initializerList.size();
    this->buffer.resize(size);

    uint32_t counter = 0;
    uint8_t* ptr = this->buffer.getDataIndex(0);
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
void PodVector<T>::resize(uint32_t newSize)
{
    resize(newSize, T());
}

template <typename T>
void PodVector<T>::resize(uint32_t newSize, const T &defaultValue)
{
    this->buffer.resize(newSize, (uint8_t *)&defaultValue);
}

template <typename T>
void PodVector<T>::uninitializedResize(uint32_t newSize)
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
uint32_t PodVector<T>::find(const T &value) const
{
    uint32_t result = 0u;
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
    this->buffer.insertIndex(this->buffer.getSize(), (uint8_t *)&obj);
}


template <typename T>
void PodVector<T>::pushBack(const PodVector<T> &vec)
{
    if (vec.size() == 0)
        return;
    uint32_t oldSize = size();
    this->buffer.resize(size() + vec.size());
    Supa::memcpy(this->buffer.getDataIndex(oldSize), vec.data(), vec.size() * sizeof(T));
}

template <typename T>
void PodVector<T>::insertIndex(uint32_t index, const T &obj)
{
    this->buffer.insertIndex(index, (const uint8_t *)&obj);
}

template <typename T>
void PodVector<T>::removeIndex(uint32_t index)
{
    this->buffer.removeIndex(index);
}

