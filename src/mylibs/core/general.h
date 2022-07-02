#pragma once

#include <core/mytypes.h>

struct Vector4;

void myPrint(const char *ptr, ...);

extern "C" int printf(const char *__restrict __format, ...);
//extern "C" void *memcpy(void *__restrict dest, const void *__restrict src, int64_t n);
//void *memcpy(void * dest, const void * src, int64_t n);
//extern "C" int memcmp( const void *ptr1, const void *ptr2, long num );
//extern "C" int strcmp(const char *str1, const char *str2);

float ftanf(float a);
float fsinf(float a);
float fcosf(float a);
float facosf(float a);
float fasinf(float a);
float ffabsf(float a);
float fsqrtf(float a);
float ffmodf(float a, float b);
float ffminf(float a, float b);
float ffmaxf(float a, float b);
float ffclampf(float a, float b, float value);


float ftand(double a);
float fsind(double a);
float fcosd(double a);
float facosd(double a);
float fasind(double a);
float ffabsd(double a);
float fsqrtd(double a);
float ffmodd(double a, double b);
float ffmind(double a, double b);
float ffmaxd(double a, double b);
float ffclampd(double a, double b, double value);


static constexpr float PI = 3.14159265358979323846264338f;


#if _MSC_VER
#define RESTRICT __restrict
#define FORCE_INLINE __forceinline
#else
#define RESTRICT __restrict
//#define RESTRICT __restrict__
#define FORCE_INLINE __attribute__((always_inline)) inline
#endif




#ifndef ARRAYSIZES
#define ARRAYSIZES(arr) (sizeof(arr) / sizeof(arr[0]))
#endif




// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a);
uint32_t getColor(const Vector4 &col);
