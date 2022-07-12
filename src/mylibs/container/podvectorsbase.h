#pragma once

#include "vectorbase.h"

namespace std
{
    template <typename T>
    class initializer_list;
}

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

    uint32_t find(const T &value) const;

    T popBack();

    void clear() { doClear(); }


    T* begin() const { return (T*)(getBegin()); }
    T* data() const  { return (T*)(getBegin()); }
    T* end() const   { return (T*)(getEnd()  ); }

    T& front() const { return *((T*)(getBegin())); }
    T& back()  const { return *((T*)(getBack() )); }

};

template <typename T>
T & PodVector<T>::operator[] (uint32_t index) const
{
    uint8_t *ptr = this->buffer.getDataIndex(index);
    ASSERT(ptr);
    return (T &)(*ptr);
}

