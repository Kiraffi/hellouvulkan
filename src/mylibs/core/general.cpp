#include "general.h"
#include "mytypes.h"

#include <math/vector3.h>

#include <math.h>
#include <string.h>

#include <stdio.h>

/*
i32 snprintf(char *const buffer, size_t bufferCount, const char *__restrict const format, ...)
{
    snprintf(buffer, bufferCount, format, ...);
}
*/

namespace Supa
{
    char *strcat(char *RESTRICT dst, const char *RESTRICT src)
    {
        return ::strcat(dst, src);
    }

    void *memcpy(void *destination, const void *source, size_t num)
    {
        return ::memcpy(destination, source, num);
    }
    void *memmove(void *destination, const void *source, size_t num)
    {
        return ::memmove(destination, source, num);
    }

    void *memset(void *destination, i32 value, size_t num)
    {
        return ::memset(destination, value, num);
    }
    i32 memcmp(const void *const ptr1, const void *const ptr2, size_t num )
    {
        return ::memcmp(ptr1, ptr2, num);
    }
    i32 strcmp(const char *const str1, const char *const str2)
    {
        return ::strcmp(str1, str2);
    }
    i32 strncmp(const char *const str1, const char *const str2, size_t num)
    {
        return ::strncmp(str1, str2, num);
    }
    i32 strlen(const char *const str)
    {
        return ::strlen(str);
    }

    i32 absi(i32 v) { return v < 0 ? -v : v; }

    u32 minu32(u32 a, u32 b) { return a < b ? a : b; }
    u32 maxu32(u32 a, u32 b) { return a > b ? a : b; }

    float atan2f(float a, float b) { return ::atan2f(a, b); }
    float sqrtf(float a) { return ::sqrtf(a); }
    float tanf(float a) { return ::tanf(a); }
    float sinf(float a) { return ::sinf(a); }
    float cosf(float a) { return ::cosf(a); }
    float asinf(float a) { return ::asinf(a); }
    float acosf(float a) { return ::acosf(a); }
    float absf(float a) { u32 b = *((u32 *)(&a)); b &= 0x7fff'ffff; return *((float *)(&b)); }
    float modf(float a, float b) { return ::fmodf(a, b); }
    float minf(float a, float b) { return (a < b) ? a : b; }
    float maxf(float a, float b) { return (a > b) ? a : b; }
    float clampf(float a, float b, float value) { return maxf(a, minf(b, value)); }
    float powf(float a, float b) { return ::powf(a, b); }
    float expd(float a) { return ::expf(a); }

    double atan2d(double a, double b) { return ::atan2(a, b); }
    double sqrtd(double a) { return ::sqrt(a); }
    double tand(double a) { return ::tan(a); }
    double sind(double a) { return ::sin(a); }
    double cosd(double a) { return ::cos(a); }
    double asind(double a) { return ::asin(a); }
    double acosd(double a) { return ::acos(a); }
    double absd(double a) { u64 b = *( ( u64 * )( &a ) ); b &= 0x7fff'ffff'ffff'ffff; return *( ( double * )( &b ) ); }
    double modd(double a, double b) { return ::fmod(a, b); }
    double mind(double a, double b) { return ( a < b ) ? a : b; }
    double maxd(double a, double b) { return ( a > b ) ? a : b; }
    double clampd(double a, double b, double value) { return maxd(a, mind(b, value)); }
    double powd(double a, double b) { return ::pow(a, b); }
    double expd(double a) { return ::exp(a); }

}


// color values r,g,h,a between [0..1]
u32 getColor(float r, float g, float b, float a)
{
    r = Supa::maxf(0.0f, Supa::minf(r, 1.0f));
    g = Supa::maxf(0.0f, Supa::minf(g, 1.0f));
    b = Supa::maxf(0.0f, Supa::minf(b, 1.0f));
    a = Supa::maxf(0.0f, Supa::minf(a, 1.0f));

    u32 c = 0u;
    c += (u32(r * 255.0f) & 255u);
    c += (u32(g * 255.0f) & 255u) << 8u;
    c += (u32(b * 255.0f) & 255u) << 16u;
    c += (u32(a * 255.0f) & 255u) << 24u;

    return c;
}

u32 getColor(const Vector4 &col)
{
    return getColor(col.x, col.y, col.z, col.w);
}

