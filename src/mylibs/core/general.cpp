#include "general.h"
#include "mytypes.h"

#include <math/vector3.h>

#include <math.h>
#include <string.h>

#include <stdio.h>

/*
int snprintf(char *const buffer, size_t bufferCount, const char *__restrict const format, ...)
{
    snprintf(buffer, bufferCount, format, ...);
}
*/

namespace Supa
{
    void *memcpy(void *destination, const void *source, size_t num)
    {
        return ::memcpy(destination, source, num);
    }
    void *memmove(void *destination, const void *source, size_t num)
    {
        return ::memmove(destination, source, num);
    }

    void *memset(void *destination, int value, size_t num)
    {
        return ::memset(destination, value, num);
    }
    int memcmp(const void *const ptr1, const void *const ptr2, size_t num )
    {
        return ::memcmp(ptr1, ptr2, num);
    }
    int strcmp(const char *const str1, const char *const str2)
    {
        return ::strcmp(str1, str2);
    }
    int strncmp(const char *const str1, const char *const str2, size_t num)
    {
        return ::strncmp(str1, str2, num);
    }
    int strlen(const char *const str)
    {
        return ::strlen(str);
    }
    int absi(int v) { return v < 0 ? -v : v; }

    float atan2f(float a, float b) { return ::atan2f(a, b); }
    float sqrtf(float a) { return ::sqrtf(a); }
    float tanf(float a) { return ::tanf(a); }
    float sinf(float a) { return ::sinf(a); }
    float cosf(float a) { return ::cosf(a); }
    float asinf(float a) { return ::asinf(a); }
    float acosf(float a) { return ::acosf(a); }
    float absf(float a) { uint32_t b = *((uint32_t *)(&a)); b &= 0x7fff'ffff; return *((float *)(&b)); }
    float modf(float a, float b) { return ::fmodf(a, b); }
    float minf(float a, float b) { return (a < b) ? a : b; }
    float maxf(float a, float b) { return (a > b) ? a : b; }
    float clampf(float a, float b, float value) { return maxf(a, minf(b, value)); }

    float atan2d(double a, double b) { return ::atan2(a, b); }
    float sqrtd(double a) { return ::sqrt(a); }
    float tand(double a) { return ::tan(a); }
    float sind(double a) { return ::sin(a); }
    float cosd(double a) { return ::cos(a); }
    float asind(double a) { return ::asin(a); }
    float acosd(double a) { return ::acos(a); }
    float absd(double a) { uint64_t b = *( ( uint64_t * )( &a ) ); b &= 0x7fff'ffff'ffff'ffff; return *( ( double * )( &b ) ); }
    float modd(double a, double b) { return ::fmod(a, b); }
    float mind(double a, double b) { return ( a < b ) ? a : b; }
    float maxd(double a, double b) { return ( a > b ) ? a : b; }
    float clampd(double a, double b, double value) { return maxd(a, mind(b, value)); }
}


// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a)
{
    r = Supa::maxf(0.0f, Supa::minf(r, 1.0f));
    g = Supa::maxf(0.0f, Supa::minf(g, 1.0f));
    b = Supa::maxf(0.0f, Supa::minf(b, 1.0f));
    a = Supa::maxf(0.0f, Supa::minf(a, 1.0f));

    uint32_t c = 0u;
    c += (uint32_t(r * 255.0f) & 255u);
    c += (uint32_t(g * 255.0f) & 255u) << 8u;
    c += (uint32_t(b * 255.0f) & 255u) << 16u;
    c += (uint32_t(a * 255.0f) & 255u) << 24u;

    return c;
}

uint32_t getColor(const Vector4 &col)
{
    return getColor(col.x, col.y, col.z, col.w);
}

