#pragma once
#include <core/mytypes.h>

#include <initializer_list>
#include <stdint.h>

template <typename T>
class ArraySliceView final
{
public:
    ArraySliceView(const T* const data, uint32_t length) : data(data), length(length) {}
    ArraySliceView(const std::initializer_list<T>& initializerList) : 
        data(initializerList.begin()), length(initializerList.size()) {}
    const T& operator[] (uint32_t index) const
    {
        ASSERT(index < length);
        ASSERT(data);
        return data[index];
    }

    uint32_t size() const { return length; }
    const T* data = nullptr;
    uint32_t length = 0u;
};