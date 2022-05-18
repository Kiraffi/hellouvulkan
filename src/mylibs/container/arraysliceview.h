#pragma once
#include <core/mytypes.h>
#include <container/podvector.h>
#include <container/vector.h>

#include <stdint.h>

template <typename T>
class ArraySliceView final
{
public:
    ArraySliceView(const T* const ptrData, uint32_t length) : ptr(ptrData), length(ptrData ? length : 0) {}
    //ArraySliceView(const std::initializer_list<T>& initializerList) :
    //    ptr(initializerList.size() ? initializerList.begin() : nullptr), length(initializerList.size()) {}

    const T* begin() const { return ptr; }
    const T* end() const { return ptr + length; }

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
    const T* data() const { return ptr; }
    const T* ptr = nullptr;
    uint32_t length = 0u;
};


template <typename T>
ArraySliceView<T> sliceFromPodVector(const PodVector<T>& v)
{
    return {v.data(), v.size()};
}

template <typename T>
ArraySliceView<T> sliceFromPodVector(const Vector<T>& v)
{
    return {v.data(), v.size()};
}