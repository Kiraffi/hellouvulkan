#pragma once

#include <core/mytypes.h>
#include <core/supa.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
    #ifndef WIN32
        #define WIN32
    #endif
#endif

#if _MSC_VER
#define RESTRICT __restrict
#define FORCE_INLINE __forceinline
#ifndef WIN32
    #define WIN32
#endif

#else
#define RESTRICT __restrict
//#define RESTRICT __restrict__
#define FORCE_INLINE __attribute__((always_inline)) inline
#endif

void myPrint(const char *ptr, ...);

extern "C" int printf(const char *__restrict __format, ...);
#if WIN32
    extern "C"  __declspec(dllexport) int snprintf(char *buffer, size_t bufferCount, const char *__format, ...);
#else
extern "C" int snprintf (char *__restrict __s, size_t __maxlen, const char *__restrict __format, ...)
    __THROWNL __attribute__ ((__format__ (__printf__, 3, 4)));
#endif

static constexpr float PI = 3.14159265358979323846264338f;





#ifndef ARRAYSIZES
#define ARRAYSIZES(arr) (sizeof(arr) / sizeof(arr[0]))
#endif




// color values r,g,h,a between [0..1]

struct Vector4;
uint32_t getColor(float r, float g, float b, float a);
uint32_t getColor(const Vector4 &col);
