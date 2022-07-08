#include "general.h"
#include "mytypes.h"

#include <math/vector3.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

/*
int snprintf(char *const buffer, size_t bufferCount, const char *__restrict const format, ...)
{
    snprintf(buffer, bufferCount, format, ...);
}
*/

float fsqrtf(float a) { return sqrtf(a); }
float ftanf(float a) { return tanf(a); }
float fsinf(float a) { return sinf(a); }
float fcosf(float a) { return cosf(a); }
float fasinf(float a) { return asinf(a); }
float facosf(float a) { return acosf(a); }
float ffabsf(float a) { uint32_t b = *((uint32_t *)(&a)); b &= 0x7fff'ffff; return *((float *)(&b)); }
float ffmodf(float a, float b) { return fmodf(a, b); }
float ffminf(float a, float b) { return (a < b) ? a : b; }
float ffmaxf(float a, float b) { return (a > b) ? a : b; }
float ffclampf(float a, float b, float value) { return ffmaxf(a, ffminf(b, value)); }

float fsqrtd(double a) { return sqrt(a); }
float ftand(double a) { return tan(a); }
float fsind(double a) { return sin(a); }
float fcosd(double a) { return cos(a); }
float fasind(double a) { return asin(a); }
float facosd(double a) { return acos(a); }
float ffabsd(double a) { uint64_t b = *( ( uint64_t * )( &a ) ); b &= 0x7fff'ffff'ffff'ffff; return *( ( double * )( &b ) ); }
float ffmodd(double a, double b) { return fmod(a, b); }
float ffmind(double a, double b) { return ( a < b ) ? a : b; }
float ffmaxd(double a, double b) { return ( a > b ) ? a : b; }
float ffclampd(double a, double b, double value) { return ffmaxd(a, ffmind(b, value)); }

// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a)
{
    r = ffmaxf(0.0f, ffminf(r, 1.0f));
    g = ffmaxf(0.0f, ffminf(g, 1.0f));
    b = ffmaxf(0.0f, ffminf(b, 1.0f));
    a = ffmaxf(0.0f, ffminf(a, 1.0f));

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

