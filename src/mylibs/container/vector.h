#pragma once

#include <initializer_list>
#include "vectorbase.h"

#include <type_traits>
#include <new>

#define CHECK_NO_POD_MACRO() \
    static constexpr bool layout = std::is_standard_layout<T>(); \
    /*static constexpr bool trivial = std::is_trivial<T>();*/ \
    static constexpr bool trivialCopy = std::is_trivially_copyable<T>(); \
    static_assert(!(layout && trivialCopy), "Pod class!");

template <typename T>
class Vector : public VectorBase
{
public:
    Vector();
    virtual ~Vector();
    //Vector(uint32_t size);

    Vector(const T *b, const T* e);
    Vector(const Vector<T> &vec);
    Vector(Vector<T> &&other) noexcept;

    Vector(const std::initializer_list<T> &initializerList);

    Vector& operator=(const Vector<T> &vec);

    T &operator[] (uint32_t index) const;

    void pushBack(const T &obj);
    void push_back(const T &obj) { pushBack(obj); }
    void emplace_back(const T &obj) { pushBack(obj); }
    void insertIndex(uint32_t index, const T &obj);
    void removeIndex(uint32_t index);

    void resize(uint32_t newSize);
    void resize(uint32_t newSize, const T &defaultValue);

    void clear();

    T *const begin() const { return (T *const)(getBegin()); }
    T *const data() const  { return (T *const)(getBegin()); }
    T *const end() const   { return (T *const)(getEnd()  ); }

    T& front() const { return *((T*)(getBegin())); }
    T& back()  const { return *((T*)(getBack() )); }

};

template <typename T>
Vector<T>::~Vector()
{
    for (T& t : *this)
    {
        t.~T();
    }
}


template <typename T>
Vector<T>::Vector() : VectorBase(sizeof(T))
{
    CHECK_NO_POD_MACRO();
}

/*
template <typename T>
Vector<T>::Vector(uint32_t size) : VectorBase(sizeof(T))
{
    CHECK_NO_POD_MACRO();
    resize(size);

}
*/

template <typename T>
Vector<T>::Vector(const T *b, const T* e) : VectorBase(sizeof(T))
{
    CHECK_NO_POD_MACRO();
    if(b >= e)
        return;
    uint32_t reserveSize = (intptr_t(e) - intptr_t(b)) / sizeof(T);
    this->buffer.resize(reserveSize);
    T *ptr = &begin();
    while(b < e)
    {
        new (ptr)T(*b);
        ++ptr;
        ++b;
    }
}

template <typename T>
Vector<T>::Vector(const Vector<T> &vec) : VectorBase(sizeof(T))
{
    CHECK_NO_POD_MACRO();
    // operator=-function
    *this = vec;
}

template <typename T>
Vector<T>::Vector(Vector &&other) noexcept : VectorBase(sizeof(T))
{
    CHECK_NO_POD_MACRO();
    clear();
    buffer.reset();
    buffer = other.buffer;
    new (&other.buffer) ByteBuffer(sizeof(T));
}

template <typename T>
Vector<T>::Vector(const std::initializer_list<T> &initializerList) : VectorBase(sizeof(T))
{
    CHECK_NO_POD_MACRO();
    uint32_t size = initializerList.size();
    this->buffer.resize(size);

    T* ptr = (T *)buffer.getDataIndex(0);
    for(const T &t : initializerList)
    {
        new(ptr)T(t);
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
        new(ptr)T(value);
        ptr++;
    }
    
    return *this;
}

template <typename T>
T & Vector<T>::operator[] (uint32_t index) const
{
    ASSERT(index < getSize());
    uint8_t *ptr = this->buffer.getDataIndex(index);
    return (T &)(*ptr);
}



template <typename T>
void Vector<T>::resize(uint32_t newSize)
{
    resize(newSize, T());
}

template <typename T>
void Vector<T>::resize(uint32_t newSize, const T &defaultValue)
{
    uint32_t currSize = getSize();
    if (newSize < currSize)
    {
        uint32_t tmpPos = newSize;
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
            new(ptr)T(defaultValue);
            ++ptr;
            ++currSize;
        }
    } 
}





template <typename T>
void Vector<T>::pushBack(const T &obj)
{
    this->buffer.insertIndex(this->buffer.getSize());
    new(&back())T(obj);
}



template <typename T>
void Vector<T>::insertIndex(uint32_t index, const T &obj)
{
    ASSERT(index < getSize());
    this->buffer.insertIndex(index);
    new(buffer.getDataIndex(index))T(obj);
}

template <typename T>
void Vector<T>::removeIndex(uint32_t index)
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
