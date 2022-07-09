
#pragma once

#include <core/mytypes.h>
#include <container/podvectorsbase.h>
#include <container/vectorsbase.h>

template <typename T>
class ArraySliceView final
{
public:
    ArraySliceView(const T *const ptrData, uint32_t length) : ptr(ptrData), length(ptrData ? length : 0) {}
    ArraySliceView(const ArraySliceView &view, uint32_t start, uint32_t length) :
        ptr(view.ptr == nullptr || start >= view.length ? view.ptr : view.ptr + start),
        length(view.ptr == nullptr || start >= view.length ? 0 :
            start + length >= view.length ? view.length - start : length)
    {
    }

    const T *const begin() const { return ptr; }
    const T *const end() const { return ptr + length; }

    const T& operator[] (uint32_t index) const
    {
        ASSERT(index < length);
        ASSERT(ptr);
        return ptr[index];
    }

    bool isValid() const
    {
        return (ptr != nullptr && length > 0u) || (ptr == nullptr && length == 0);
    }

    uint32_t size() const { return length; }
    const T *const data() const { return ptr; }
    const T *const ptr = nullptr;
    uint32_t length = 0u;
};

template <typename T>
class ArraySliceViewMutable final
{
public:
    ArraySliceViewMutable(T *const ptrData, uint32_t length) : ptr(ptrData), length(ptrData ? length : 0) {}
    ArraySliceViewMutable(const ArraySliceViewMutable &view, uint32_t start, uint32_t length) :
        ptr(view.ptr == nullptr || start >= view.length ? view.ptr : view.ptr + start),
        length(view.ptr == nullptr || start >= view.length ? 0 :
            start + length >= view.length ? view.length - start : length) {}

    T *const begin() const { return ptr; }
    T *const end() const { return ptr + length; }

    T &operator[] (uint32_t index)
    {
        ASSERT(index < length);
        ASSERT(ptr);
        return ptr[index];
    }

    bool isValid() const
    {
        return (ptr != nullptr && length > 0u) || (ptr == nullptr && length == 0);
    }

    uint32_t size() const { return length; }
    T *const data() const { return ptr; }
    T *const ptr = nullptr;
    uint32_t length = 0u;
};


class ArraySliceViewBytes final
{
public:
    ArraySliceViewBytes(const void* const ptrData, uint32_t length, uint32_t typeSize) :
        ptr((const uint8_t *const)ptrData),
        length(ptrData ? length : 0),
        dataTypeSize(typeSize) {}

    template <typename T>
    ArraySliceViewBytes(const T* const ptrData, uint32_t length) :
        ptr((const uint8_t *const)ptrData),
        length(ptrData ? length : 0),
        dataTypeSize(sizeof(T)) {}

    template <typename T>
    ArraySliceViewBytes(ArraySliceView<T> view) :
        ptr((const uint8_t *const)view.begin()),
        length(view.length),
        dataTypeSize(sizeof(T)) {}

    const uint8_t *const begin() const { return ptr; }
    const uint8_t *const end() const { return ptr + dataTypeSize * length; }

    const uint8_t* operator[] (uint32_t index) const
    {
        ASSERT(index < length);
        ASSERT(ptr);
        return &ptr[index * dataTypeSize];
    }

    bool isValid() const
    {
        return (ptr != nullptr && length > 0u && dataTypeSize > 0u) || (ptr == nullptr && length == 0);
    }

    uint32_t size() const { return length; }
    const uint8_t *const data() const { return ptr; }
    const uint8_t *const ptr = nullptr;
    uint32_t length = 0u;
    uint32_t dataTypeSize = 0u;
};


class ArraySliceViewBytesMutable final
{
public:
    ArraySliceViewBytesMutable(const void *ptrData, uint32_t length, uint32_t typeSize) :
        ptr((uint8_t *)ptrData),
        length(ptrData ? length : 0),
        dataTypeSize(typeSize) {}

    template <typename T>
    ArraySliceViewBytesMutable(const T *ptrData, uint32_t length) :
        ptr((uint8_t *)ptrData),
        length(ptrData ? length : 0),
        dataTypeSize(sizeof(T)) {}

    template <typename T>
    ArraySliceViewBytesMutable(ArraySliceViewMutable<T> view) :
        ptr((uint8_t *)view.begin()),
        length(view.length),
        dataTypeSize(sizeof(T)) {}

    uint8_t *const begin() const { return ptr; }
    uint8_t *const end() const { return ptr + dataTypeSize * length; }

    const uint8_t *operator[] (uint32_t index) const
    {
        ASSERT(index < length);
        ASSERT(ptr);
        return &ptr[index * dataTypeSize];
    }

    bool isValid() const
    {
        return (ptr != nullptr && length > 0u && dataTypeSize > 0u) || (ptr == nullptr && length == 0);
    }

    uint32_t size() const { return length; }
    uint8_t *const data() const { return ptr; }
    uint8_t *const ptr = nullptr;
    uint32_t length = 0u;
    uint32_t dataTypeSize = 0u;
};



template <typename T>
ArraySliceView<T> sliceFromPodVector(const PodVector<T>& v)
{
    return {v.data(), v.size()};
}

template <typename T>
ArraySliceView<T> sliceFromVector(const Vector<T>& v)
{
    return {v.data(), v.size()};
}


template <typename T>
ArraySliceViewBytes sliceFromPodVectorBytes(const PodVector<T>& v)
{
    return {v.data(), v.size()};
}

template <typename T>
ArraySliceViewBytes sliceFromVectorBytes(const Vector<T>& v)
{
    return {v.data(), v.size()};
}




template <typename T>
ArraySliceViewMutable<T> sliceFromPodVectorMutable(PodVector<T> &v)
{
    return { v.data(), v.size() };
}

template <typename T>
ArraySliceViewMutable<T> sliceFromVectorMutable(Vector<T> &v)
{
    return { v.data(), v.size() };
}

template <typename T>
ArraySliceViewBytesMutable sliceFromPodVectorBytesMutable(PodVector<T> &v)
{
    return { v.data(), v.size() };
}

template <typename T>
ArraySliceViewBytesMutable sliceFromVectorBytesMutable(Vector<T> &v)
{
    return { v.data(), v.size() };
}
