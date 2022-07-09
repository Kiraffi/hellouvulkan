#pragma once

#include <core/mytypes.h>

namespace Supa
{
    void *memcpy(void *destination, const void *source, size_t num);
    void *memmove(void *destination, const void *source, size_t num);

    void *memset(void *destination, int value, size_t num);
    int memcmp(const void *const ptr1, const void *const ptr2, size_t num );
    int strcmp(const char *const str1, const char *const str2);
    int strncmp(const char *const str1, const char *const str2, size_t num);
    int snprintf(char *buffer, size_t bufferCount, const char *format, ...);
    int strlen(const char *const str);

    int absi(int v);

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
