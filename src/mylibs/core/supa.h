#pragma once

#include <core/mytypes.h>

namespace Supa
{
    char *strcat(char*__restrict dst, const char*__restrict src);
    void *memcpy(void* destination, const void *source, size_t num);
    void *memmove(void *destination, const void *source, size_t num);

    void *memset(void *destination, i32 value, size_t num);
    i32 memcmp(const void *const ptr1, const void *const ptr2, size_t num );
    i32 strcmp(const char *const str1, const char *const str2);
    i32 strncmp(const char *const str1, const char *const str2, size_t num);
    i32 snprintf(char *buffer, size_t bufferCount, const char *format, ...);
    i32 strlen(const char *const str);

    i32 absi(i32 v);

    u32 minu32(u32 a, u32 b);
    u32 maxu32(u32 a, u32 b);

    float atan2f(float a, float b);
    float tanf(float a);
    float sinf(float a);
    float cosf(float a);
    float acosf(float a);
    float asinf(float a);
    float absf(float a);
    float sqrtf(float a);
    float modf(float a, float b);
    float minf(float a, float b);
    float maxf(float a, float b);
    float clampf(float a, float b, float value);
    float powf(float a, float b);
    float expf(float a);

    double atan2d(double a, double b);
    double tand(double a);
    double sind(double a);
    double cosd(double a);
    double acosd(double a);
    double asind(double a);
    double absd(double a);
    double sqrtd(double a);
    double modd(double a, double b);
    double mind(double a, double b);
    double maxd(double a, double b);
    double clampd(double a, double b, double value);

    double powd(double a, double b);
    double expd(double a);

}
