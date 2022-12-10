
#pragma once

#include <core/mytypes.h>
#include <container/podvectorsbase.h>
#include <container/vectorsbase.h>

template <typename T>
class ArraySliceView final
{
public:
    ArraySliceView(const T *const ptrData, u32 length) : ptr(ptrData), length(ptrData ? length : 0) {}
    ArraySliceView(const ArraySliceView &view, u32 start, u32 length) :
        ptr(view.ptr == nullptr || start >= view.length ? view.ptr : view.ptr + start),
        length(view.ptr == nullptr || start >= view.length ? 0 :
            start + length >= view.length ? view.length - start : length)
    {
    }

    const T *const begin() const { return ptr; }
    const T *const end() const { return ptr + length; }

    const T& operator[] (u32 index) const
    {
        ASSERT(index < length);
        ASSERT(ptr);
        return ptr[index];
    }

    bool isValid() const
    {
        return (ptr != nullptr && length > 0u) || (ptr == nullptr && length == 0);
    }

    u32 size() const { return length; }
    const T *const data() const { return ptr; }
    const T *const ptr = nullptr;
    u32 length = 0u;
};

template <typename T>
class ArraySliceViewMutable final
{
public:
    ArraySliceViewMutable(T *const ptrData, u32 length) : ptr(ptrData), length(ptrData ? length : 0) {}
    ArraySliceViewMutable(const ArraySliceViewMutable &view, u32 start, u32 length) :
        ptr(view.ptr == nullptr || start >= view.length ? view.ptr : view.ptr + start),
        length(view.ptr == nullptr || start >= view.length ? 0 :
            start + length >= view.length ? view.length - start : length) {}

    T *const begin() const { return ptr; }
    T *const end() const { return ptr + length; }

    T &operator[] (u32 index)
    {
        ASSERT(index < length);
        ASSERT(ptr);
        return ptr[index];
    }

    bool isValid() const
    {
        return (ptr != nullptr && length > 0u) || (ptr == nullptr && length == 0);
    }

    u32 size() const { return length; }
    T *const data() const { return ptr; }
    T *const ptr = nullptr;
    u32 length = 0u;
};


class ArraySliceViewBytes final
{
public:
    ArraySliceViewBytes(const void* const ptrData, u32 length, u32 typeSize) :
        ptr((const u8 *const)ptrData),
        length(ptrData ? length : 0),
        dataTypeSize(typeSize) {}

    template <typename T>
    ArraySliceViewBytes(const T* const ptrData, u32 length) :
        ptr((const u8 *const)ptrData),
        length(ptrData ? length : 0),
        dataTypeSize(sizeof(T)) {}

    template <typename T>
    ArraySliceViewBytes(ArraySliceView<T> view) :
        ptr((const u8 *const)view.begin()),
        length(view.length),
        dataTypeSize(sizeof(T)) {}

    const u8 *const begin() const { return ptr; }
    const u8 *const end() const { return ptr + dataTypeSize * length; }

    const u8* operator[] (u32 index) const
    {
        ASSERT(index < length);
        ASSERT(ptr);
        return &ptr[index * dataTypeSize];
    }

    bool isValid() const
    {
        return (ptr != nullptr && length > 0u && dataTypeSize > 0u) || (ptr == nullptr && length == 0);
    }

    u32 size() const { return length; }
    const u8 *const data() const { return ptr; }
    const u8 *const ptr = nullptr;
    u32 length = 0u;
    u32 dataTypeSize = 0u;
};


class ArraySliceViewBytesMutable final
{
public:
    ArraySliceViewBytesMutable(const void *ptrData, u32 length, u32 typeSize) :
        ptr((u8 *)ptrData),
        length(ptrData ? length : 0),
        dataTypeSize(typeSize) {}

    template <typename T>
    ArraySliceViewBytesMutable(const T *ptrData, u32 length) :
        ptr((u8 *)ptrData),
        length(ptrData ? length : 0),
        dataTypeSize(sizeof(T)) {}

    template <typename T>
    ArraySliceViewBytesMutable(ArraySliceViewMutable<T> view) :
        ptr((u8 *)view.begin()),
        length(view.length),
        dataTypeSize(sizeof(T)) {}

    u8 *const begin() const { return ptr; }
    u8 *const end() const { return ptr + dataTypeSize * length; }

    const u8 *operator[] (u32 index) const
    {
        ASSERT(index < length);
        ASSERT(ptr);
        return &ptr[index * dataTypeSize];
    }

    bool isValid() const
    {
        return (ptr != nullptr && length > 0u && dataTypeSize > 0u) || (ptr == nullptr && length == 0);
    }

    u32 size() const { return length; }
    u8 *const data() const { return ptr; }
    u8 *const ptr = nullptr;
    u32 length = 0u;
    u32 dataTypeSize = 0u;
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
