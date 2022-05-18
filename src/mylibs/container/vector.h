#pragma once

#include <initializer_list>
#include "vectorbase.h"

#define CHECK_NO_POD_MACRO() \
    static constexpr bool layout = !std::is_standard_layout<T>(); \
    /*static constexpr bool trivial = std::is_trivial<T>();*/ \
    static constexpr bool trivialCopy = !std::is_trivially_copyable<T>(); \
    static_assert(layout && trivialCopy, "Pod class!");


template <typename T>
class Vector : public VectorBase
{
public:
    Vector();
    virtual ~Vector();
    Vector(uint32_t size);

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

    T* begin() const { return (T*)(getBegin()); }
    T* data() const  { return (T*)(getBegin()); }
    T* end() const   { return (T*)(getEnd()  ); }

    T& front() const { return *((T*)(getBegin())); }
    T& back()  const { return *((T*)(getBack() )); }

};

template <typename T>
Vector<T>::~Vector()
{
    if(buffer.getSize() == 0)
        return;

    T *ptr = (T *)this->buffer.getDataIndex(0);
    for(uint32_t i = 0; i < buffer.getSize(); ++i)
    {
        ptr->~T();
        ++ptr;
    }
}


template <typename T>
Vector<T>::Vector() : VectorBase(sizeof(T))
{
    CHECK_NO_POD_MACRO();
}

template <typename T>
Vector<T>::Vector(uint32_t size) : VectorBase(sizeof(T))
{
    CHECK_NO_POD_MACRO();
    this->buffer.resize(size);
}

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
        new (ptr)(T);
        *ptr = *b;
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

    new (&this->buffer) ByteBuffer(sizeof(T));
    memmove(&this->buffer, &other.buffer, sizeof(ByteBuffer));

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
        new(ptr)(T);
        *ptr = t;
        ++ptr;
    }
}



template <typename T>
Vector<T>& Vector<T>::operator=(const Vector<T> &vec)
{
    CHECK_NO_POD_MACRO();

    if (&vec == this)
        return *this;

    this->~Vector<T>();

    new (&this->buffer) ByteBuffer(sizeof(T));
    buffer.resize(vec.size());
    T* ptr = (T *)this->buffer.getDataIndex(0);
    for(T &value : vec)
    {
        new(ptr)(T);
        *ptr = value;
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
    if(getSize() >= newSize)
        return;
    this->buffer.resize(newSize);
}

template <typename T>
void Vector<T>::resize(uint32_t newSize, const T &defaultValue)
{
    uint32_t currSize = getSize();
    if(currSize >= newSize)
        return;

    resize(newSize);
    T* ptr = &buffer.getDataIndex(currSize);
    while(currSize < newSize)
    {
        *ptr = defaultValue;
        ++ptr;
        ++currSize;
    }
}





template <typename T>
void Vector<T>::pushBack(const T &obj)
{
    this->buffer.insertIndex(this->buffer.getSize(), (uint8_t *)&obj);
    new(&back())(T);
    back() = obj;
}



template <typename T>
void Vector<T>::insertIndex(uint32_t index, const T &obj)
{
    ASSERT(index < getSize());
    this->buffer.insertIndex(index, (const uint8_t *)&obj);
    new(buffer.getDataIndex(index))(T);
    *((T *)buffer.getDataIndex(index))  = obj;
}

template <typename T>
void Vector<T>::removeIndex(uint32_t index)
{
    ASSERT(index < getSize());
    T *t = (T *)buffer.getDataIndex(index);
    t->~T();
    this->buffer.removeIndex(index);
}

