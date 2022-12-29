#pragma once

#include "podvectortype.h"
#include "vectorbase.h"
#include "vectorsbase.h"

#include <initializer_list>
//#include <new>

#define CHECK_NO_POD_MACRO() isNotPodType<T>();

template <typename T>
Vector<T>::Vector() : VectorBase(sizeof(T), BufferType::VECTOR)
{
    CHECK_NO_POD_MACRO();
}

/*
template <typename T>
Vector<T>::Vector(u32 size) : VectorBase(sizeof(T), BufferType::VECTOR)
{
    CHECK_NO_POD_MACRO();
    resize(size);

}
*/

template <typename T>
Vector<T>::Vector(const T *b, const T* e) : VectorBase(sizeof(T), BufferType::VECTOR)
{
    CHECK_NO_POD_MACRO();
    if(b >= e)
        return;
    u32 reserveSize = (intptr_t(e) - intptr_t(b)) / sizeof(T);
    this->buffer.resize(reserveSize);
    T *ptr = &begin();
    while(b < e)
    {
        newInPlacement(ptr, *b);
        ++ptr;
        ++b;
    }
}

template <typename T>
Vector<T>::Vector(const Vector<T> &vec) : VectorBase(sizeof(T), BufferType::VECTOR)
{
    CHECK_NO_POD_MACRO();
    // operator=-function
    *this = vec;
}

template <typename T>
Vector<T>::Vector(Vector &&other) noexcept : VectorBase(sizeof(T), BufferType::VECTOR)
{
    CHECK_NO_POD_MACRO();
    clear();
    buffer.reset();
    buffer = other.buffer;
    other.buffer = ByteBuffer(sizeof(T), BufferType::VECTOR);
}

template <typename T>
Vector<T>::Vector(const std::initializer_list<T> &initializerList) :
    VectorBase(sizeof(T), BufferType::VECTOR)
{
    CHECK_NO_POD_MACRO();
    u32 size = initializerList.size();
    this->buffer.resize(size);

    T* ptr = (T *)buffer.getDataIndex(0);
    for(const T &t : initializerList)
    {
        newInPlace(ptr, t);
        ++ptr;
    }
}



template <typename T>
Vector<T>& Vector<T>::operator=(const Vector<T> &vec)
{
    CHECK_NO_POD_MACRO();

    if (&vec == this || vec.buffer.getBegin() == buffer.getBegin())
        return *this;

    clear();
    buffer.reset();
    ASSERT(sizeof(T) == buffer.getDataSize());
    if(vec.size() == 0)
        return *this;
    buffer.resize(vec.size());
    T* ptr = (T *)buffer.getBegin();
    for(T &value : vec)
    {
        newInPlace(ptr, value);
        ptr++;
    }

    return *this;
}



template <typename T>
void Vector<T>::resize(u32 newSize)
{
    resize(newSize, T());
}

template <typename T>
void Vector<T>::resize(u32 newSize, const T &defaultValue)
{
    u32 currSize = getSize();
    if (newSize < currSize)
    {
        u32 tmpPos = newSize;
        T* ptr = (T*)buffer.getDataIndex(tmpPos);
        while (tmpPos < currSize)
        {
            ptr->~T();
            ++ptr;
            ++tmpPos;
        }
    }
    this->buffer.resize(newSize);
    if(currSize < newSize)
    {
        T* ptr = (T *)buffer.getDataIndex(currSize);
        while(currSize < newSize)
        {
            newInPlace(ptr, defaultValue);
            ++ptr;
            ++currSize;
        }
    }
}





template <typename T>
void Vector<T>::pushBack(const T &obj)
{
    this->buffer.insertIndex(this->buffer.getSize());
    T *ptr = &back();
    newInPlace(ptr, obj);

}



template <typename T>
void Vector<T>::insertIndex(u32 index, const T &obj)
{
    ASSERT(index < getSize());
    this->buffer.insertIndex(index);
    newInPlace((T *)buffer.getDataIndex(index), obj);
}

template <typename T>
void Vector<T>::removeIndex(u32 index)
{
    ASSERT(index < getSize());
    T *t = (T *)buffer.getDataIndex(index);
    t->~T();
    this->buffer.removeIndex(index);
}

template <typename T>
void Vector<T>::clear()
{
    for(T &t : *this)
    {
        t.~T();
    }
    doClear();
}
