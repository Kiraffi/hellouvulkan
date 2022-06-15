#pragma once

#include <initializer_list>
#include "vectorbase.h"

#define CHECK_POD_MACRO() \
    static constexpr bool layout = std::is_standard_layout<T>(); \
    /*static constexpr bool trivial = std::is_trivial<T>();*/ \
    static constexpr bool trivialCopy = std::is_trivially_copyable<T>(); \
    static_assert(layout && trivialCopy, "Not pod class!");


template <typename T>
class PodVector : public VectorBase
{
public:
    PodVector();
    
    //PodVector(uint32_t size);

    PodVector(const T *b, const T* e);
    PodVector(const PodVector<T> &vec);
    PodVector(PodVector<T> &&other) noexcept;

    PodVector(const std::initializer_list<T> &initializerList);

    PodVector& operator=(const PodVector<T> &vec);

    T &operator[] (uint32_t index) const;

    void pushBack(const T &obj);
    void pushBack(const PodVector<T> &obj);
    void push_back(const T &obj) { pushBack(obj); }
    void emplace_back(const T &obj) { pushBack(obj); }
    void insertIndex(uint32_t index, const T &obj);
    void removeIndex(uint32_t index);

    void resize(uint32_t newSize);
    void resize(uint32_t newSize, const T &defaultValue);
    void uninitializedResize(uint32_t newSize);

    T popBack();

    void clear() { doClear(); }

    T* begin() const { return (T*)(getBegin()); }
    T* data() const  { return (T*)(getBegin()); }
    T* end() const   { return (T*)(getEnd()  ); }

    T& front() const { return *((T*)(getBegin())); }
    T& back()  const { return *((T*)(getBack() )); }

};


template <typename T>
PodVector<T>::PodVector() : VectorBase(sizeof(T))
{
    CHECK_POD_MACRO();
}

/*
template <typename T>
PodVector<T>::PodVector(uint32_t size) : VectorBase(sizeof(T))
{
    CHECK_POD_MACRO();
    const T t{};
    this->buffer.resize(size, (uint8_t *)&t);
}
*/

template <typename T>
PodVector<T>::PodVector(const T *b, const T* e) : VectorBase(sizeof(T))
{
    CHECK_POD_MACRO();
    ASSERT(b < e);
    int32_t reserveSize = (intptr_t(e) - intptr_t(b)) / sizeof(T);
    ASSERT(reserveSize > 0);

    this->buffer.resize(reserveSize);
    memcpy(this->buffer.getDataIndex(0), b, reserveSize * sizeof(T));
}

template <typename T>
PodVector<T>::PodVector(const PodVector<T> &vec) : VectorBase(sizeof(T))
{
    CHECK_POD_MACRO();
    // operator=-function
    *this = vec;
}

template <typename T>
PodVector<T>::PodVector(PodVector &&other) noexcept : VectorBase(sizeof(T))
{
    CHECK_POD_MACRO();
    buffer.reset();
    buffer = other.buffer;
    new (&other.buffer) ByteBuffer(sizeof(T));
}

template <typename T>
PodVector<T>::PodVector(const std::initializer_list<T> &initializerList) : VectorBase(sizeof(T))
{
    CHECK_POD_MACRO();
    uint32_t size = initializerList.size();
    this->buffer.resize(size);

    uint32_t counter = 0;
    uint8_t* ptr = this->buffer.getDataIndex(0);
    memcpy(ptr, &*initializerList.begin(), size * sizeof(T));
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
T & PodVector<T>::operator[] (uint32_t index) const
{
    uint8_t *ptr = this->buffer.getDataIndex(index);
    return (T &)(*ptr);
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
    memcpy(this->buffer.getDataIndex(oldSize), vec.data(), vec.size() * sizeof(T));
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

