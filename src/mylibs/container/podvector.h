#pragma once

#include "bytebuffer.h"
#include <memory>
#include <initializer_list>
#include <string.h>

#define CHECK_POD_MACRO() \
    static constexpr bool layout = std::is_standard_layout<T>(); \
    /*static constexpr bool trivial = std::is_trivial<T>();*/ \
    static constexpr bool trivialCopy = std::is_trivially_copyable<T>(); \
    static_assert(layout && trivialCopy, "Not pod class!");


template <typename T>
class PodVector final
{
public:
    PodVector();
    // : buffer(sizeof(T))
    //{
    //    static constexpr bool layout = std::is_standard_layout<T>();
    //    //static constexpr bool trivial = std::is_trivial<T>();
    //    static constexpr bool trivialCopy = std::is_trivially_copyable<T>();
    //    static_assert(layout && trivialCopy, "Not pod class!");
    //}
    ~PodVector();
    PodVector(uint32_t size);

    PodVector(const T *b, const T* e);
    PodVector(const PodVector<T> &vec);
    PodVector(PodVector<T> &&other);

    PodVector(const std::initializer_list<T> &initializerList);

    PodVector& operator=(const PodVector<T> &vec);

    T &operator[] (uint32_t index) const;

    void pushBack(const T &obj);
    void push_back(const T &obj) { pushBack(obj); }
    void emplace_back(const T &obj) { pushBack(obj); }
    void insertIndex(uint32_t index, const T &obj);
    void removeIndex(uint32_t index);

    void clear();
    void resize(uint32_t newSize);
    void resize(uint32_t newSize, const T &defaultValue);

    void reserve(uint32_t newSize);
    T* begin() const;
    T* end() const;
    T* data() const;
    T& back() const;

    uint32_t size() const;
    uint32_t getSize() const { return size(); }
    uint32_t getCapasity() const;
    bool empty() const { return size() == 0u; }

private:
    ByteBuffer buffer;
};


template <typename T>
PodVector<T>::PodVector() : buffer(sizeof(T))
{
    CHECK_POD_MACRO();
}
template <typename T>
PodVector<T>::~PodVector()
{
    buffer.~ByteBuffer();
}

template <typename T>
PodVector<T>::PodVector(uint32_t size) : buffer(sizeof(T))
{
    CHECK_POD_MACRO();
    buffer.resize(size);
}

template <typename T>
PodVector<T>::PodVector(const T *b, const T* e) : buffer(sizeof(T))
{
    CHECK_POD_MACRO();
    if(b >= e)
        return;
    uint32_t counter = 0;
    uint32_t reserveSize = (intptr_t(e) - intptr_t(b)) / sizeof(T);
    buffer.reserve(reserveSize);
    while(b < e)
    {
        buffer.insertIndex(counter, (const uint8_t *)b);
        counter++;
        b += 1;
    }
}

template <typename T>
PodVector<T>::PodVector(const PodVector<T> &vec) : buffer(sizeof(T))
{
    CHECK_POD_MACRO();

    *this = vec;
}

template <typename T>
PodVector<T>::PodVector(PodVector &&other) : buffer(sizeof(T))
{
    CHECK_POD_MACRO();

    new (&buffer) ByteBuffer(sizeof(T));
    memmove(&buffer, &other.buffer, sizeof(ByteBuffer));

    new (&other.buffer) ByteBuffer(sizeof(T));
}

template <typename T>
PodVector<T>::PodVector(const std::initializer_list<T> &initializerList) : buffer(sizeof(T))
{
    CHECK_POD_MACRO();
    uint32_t size = initializerList.size();
    buffer.reserve(size);

    uint32_t counter = 0;

    for(const T &t : initializerList)
    {
        buffer.insertIndex(counter, (const uint8_t *)&t);
        ++counter;
    }
}



template <typename T>
PodVector<T>& PodVector<T>::operator=(const PodVector<T> &vec)
{
    CHECK_POD_MACRO();

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
T* PodVector<T>::begin() const
{
    return (T*)buffer.getBegin();
}

template <typename T>
T* PodVector<T>::data() const
{
    return (T*)buffer.getBegin();
}

template <typename T>
T* PodVector<T>::end() const
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
T& PodVector<T>::back() const
{
    if(buffer.getSize() == 0)
        return nullptr;

    uint8_t *ptr = buffer.getBegin();
    ptr += sizeof(T) * (buffer.getSize() - 1);
    return *(T*)(ptr);
}





template <typename T>
uint32_t PodVector<T>::size() const
{
    return buffer.getSize();
}

template <typename T>
uint32_t PodVector<T>::getCapasity() const
{
    return buffer.getCapasity();
}

