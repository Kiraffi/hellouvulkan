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


class ArraySliceViewBytes final
{
public:
    ArraySliceViewBytes(const void* const ptrData, uint32_t length, uint32_t typeSize) :
        ptr((uint8_t *)ptrData),
        length(ptrData ? length : 0),
        dataTypeSize(typeSize) {}

    template <typename T>
    ArraySliceViewBytes(const T* const ptrData, uint32_t length) :
        ptr((uint8_t *)ptrData),
        length(ptrData ? length : 0),
        dataTypeSize(sizeof(T)) {}

    template <typename T>
    ArraySliceViewBytes(ArraySliceView<T> view) :
        ptr((uint8_t *)view.begin()),
        length(view.length),
        dataTypeSize(sizeof(T)) {}

    const uint8_t* begin() const { return ptr; }
    const uint8_t* end() const { return ptr + dataTypeSize * length; }

    uint8_t* begin() { return ptr; }
    uint8_t* end() { return ptr + dataTypeSize * length; }

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
    const uint8_t* data() const { return ptr; }
    uint8_t* ptr = nullptr;
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