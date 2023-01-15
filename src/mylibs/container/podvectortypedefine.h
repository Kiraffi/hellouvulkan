#pragma once

#include "podvectortype.h"
#include <new>
#include "core/mytypes.h"

// Default placement versions of operator new.
//[[__nodiscard__]] inline void* operator new(size_t, void* __p) noexcept
//{ return __p; }
//[[__nodiscard__]] inline void* operator new[](size_t, void* __p) noexcept
//{ return __p; }
// Default placement versions of operator delete.
//inline void operator delete   (void*, void*) noexcept { }
//inline void operator delete[] (void*, void*) noexcept { }

template <typename T>
void newInPlace(T *ptr, const T &value)
{
    new(ptr)T(value);
}

