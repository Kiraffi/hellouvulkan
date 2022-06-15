#pragma once

#include "vector3.h"

#include <core/assert.h>
#include <core/general.h>
#include <core/log.h>

#include <math.h>

/*
Vector2 operator+(const Vector2 &a, const Vector2 &b);
Vector2 operator+(const Vector2 &a, float value);
Vector2 operator+(float value, const Vector2 &a);
Vector2 operator-(const Vector2 &a);
Vector2 operator-(const Vector2 &a, const Vector2 &b);
Vector2 operator-(const Vector2 &a, float value);
Vector2 operator*(const Vector2 &a, float value);
Vector2 operator*(float value, const Vector2 &a);
Vector2 operator*(const Vector2 &a, const Vector2 &b);
Vector2 operator/(const Vector2 &a,float value);
Vector2 operator/(const Vector2 &a, const Vector2 &b);
Vector2 operator/(float value, const Vector2 &a);
Vector2 min(const Vector2 &v1, const Vector2 &v2);
Vector2 max(const Vector2 &v1, const Vector2 &v2);
float min(const Vector2 &v1);
float max(const Vector2 &v1);
float dot(const Vector2 &a, const Vector2 &b);
float sqrLen(const Vector2 &a);
float len(const Vector2 &a);
Vector2 lerp(const Vector2 &a, const Vector2 &b, float t);
Vector2 normalize(const Vector2 &a);

Vector3 operator+(const Vector3 &a, const Vector3 &b);
Vector3 operator+(const Vector3 &a, float value);
Vector3 operator+(float value, const Vector3 &a);
Vector3 operator-(const Vector3 &a);
Vector3 operator-(const Vector3 &a, const Vector3 &b);
Vector3 operator-(const Vector3 &a, float value);
Vector3 operator*(const Vector3 &a, float value);
Vector3 operator*(float value, const Vector3 &a);
Vector3 operator*(const Vector3 &a, const Vector3 &b);
Vector3 operator/(const Vector3 &a, float value);
Vector3 operator/(const Vector3 &a, const Vector3 &b);
Vector3 operator/(float value, const Vector3 &a);
Vector3 min(const Vector3 &v1, const Vector3 &v2);
Vector3 max(const Vector3 &v1, const Vector3 &v2);
float min(const Vector3 &v1);
float max(const Vector3 &v1);
float dot(const Vector3 &a, const Vector3 &b);
float sqrLen(const Vector3 &a);
float len(const Vector3 &a);
Vector3 lerp(const Vector3 &a, const Vector3 &b, float t);
Vector3 normalize(const Vector3 &a);
Vector3 cross(const Vector3 &a, const Vector3 &b);
Vector3 proj(const Vector3 &a, const Vector3 &b);
Vector3 reject(const Vector3 &a, const Vector3 &b);


Vector4 operator+(const Vector4 &a, const Vector4 &b);
Vector4 operator+(const Vector4 &a, float value);
Vector4 operator+(float value, const Vector4 &a);
Vector4 operator-(const Vector4 &a);
Vector4 operator-(const Vector4 &a, const Vector4 &b);
Vector4 operator-(const Vector4 &a, float value);
Vector4 operator*(const Vector4 &a, float value);
Vector4 operator*(float value, const Vector4 &a);
Vector4 operator*(const Vector4 &a, const Vector4 &b);
Vector4 operator/(const Vector4 &a, float value);
Vector4 operator/(const Vector4 &a, const Vector4 &b);
Vector4 operator/(float value, const Vector4 &a);
Vector4 min(const Vector4 &v1, const Vector4 &v2);
Vector4 max(const Vector4 &v1, const Vector4 &v2);
float min(const Vector4 &v1);
float max(const Vector4 &v1);
float dot(const Vector4 &a, const Vector4 &b);
float sqrLen(const Vector4 &a);
float len(const Vector4 &a);
Vector4 lerp(const Vector4 &a, const Vector4 &b, float t);
Vector4 normalize(const Vector4 &a);
*/


static FORCE_INLINE Vector2 operator+(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

static FORCE_INLINE Vector2 operator+(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x + value;
    result.y = a.y + value;
    return result;
}

static FORCE_INLINE Vector2 operator+(float value, const Vector2 &a)
{
    return a + value;
}


static FORCE_INLINE Vector2 operator-(const Vector2 &a)
{
    Vector2 result{ Uninit };
    result.x = -a.x;
    result.y = -a.y;
    return result;
}


static FORCE_INLINE Vector2 operator-(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

static FORCE_INLINE Vector2 operator-(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x - value;
    result.y = a.y - value;
    return result;
}

static FORCE_INLINE Vector2 operator*(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    return result;
}

static FORCE_INLINE Vector2 operator*(float value, const Vector2 &a)
{
    Vector2 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    return result;
}

static FORCE_INLINE Vector2 operator*(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

static FORCE_INLINE Vector2 operator/(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x / value;
    result.y = a.y / value;
    return result;
}

static FORCE_INLINE Vector2 operator/(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

static FORCE_INLINE Vector2 operator/(float value, const Vector2 &a)
{
    Vector2 result{ Uninit };
    result.x = value / a.x;
    result.y = value / a.y;
    return result;
}


static FORCE_INLINE Vector2 minVec(const Vector2 &v1, const Vector2 &v2)
{
    Vector2 result{ Uninit };
    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);
    return result;
}

static FORCE_INLINE Vector2 maxVec(const Vector2 &v1, const Vector2 &v2)
{
    Vector2 result{ Uninit };
    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);
    return result;
}

static FORCE_INLINE float minVec(const Vector2 &v1)
{
    return fminf(v1.x, v1.y);
}

static FORCE_INLINE float maxVec(const Vector2 &v1)
{
    return fmaxf(v1.x, v1.y);
}


static FORCE_INLINE float dot(const Vector2 &a, const Vector2 &b)
{
    return a.x * b.x + a.y * b.y;
}

static FORCE_INLINE float sqrLen(const Vector2 &a)
{
    return dot(a, a);
}

static FORCE_INLINE float len(const Vector2 &a)
{
    return fsqrtf(a.x * a.x + a.y * a.y);
}

static FORCE_INLINE Vector2 lerp(const Vector2 &a, const Vector2 &b, float t)
{
    Vec2 result{ Uninit };
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    return result;
}


static FORCE_INLINE Vector2 normalize(const Vector2 &a)
{
    if(a.x == 0.0f && a.y == 0.0f)
    {
        ASSERT(false && "Vector2 0 on normalize");
        return Vector2();
    }
    float l = sqrLen(a);
    l = fsqrtf(l);
    float perLen = 1.0f / l;
    return Vector2(a.x * perLen, a.y * perLen);
}










static FORCE_INLINE Vector3 operator+(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}
static FORCE_INLINE Vector3 operator+(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x + value;
    result.y = a.y + value;
    result.z = a.z + value;
    return result;
}
static FORCE_INLINE Vector3 operator+(float value, const Vector3 &a)
{
    return a + value;
}


static FORCE_INLINE Vector3 operator-(const Vector3 &a)
{
    Vector3 result{ Uninit };
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    return result;
}


static FORCE_INLINE Vector3 operator-(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

static FORCE_INLINE Vector3 operator-(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x - value;
    result.y = a.y - value;
    result.z = a.z - value;
    return result;
}

static FORCE_INLINE Vector3 operator*(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    return result;
}

static FORCE_INLINE Vector3 operator*(float value, const Vector3 &a)
{
    Vector3 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    return result;
}

static FORCE_INLINE Vector3 operator*(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}


static FORCE_INLINE Vector3 operator/(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x / value;
    result.y = a.y / value;
    result.z = a.z / value;
    return result;
}

static FORCE_INLINE Vector3 operator/(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

static FORCE_INLINE Vector3 operator/(float value, const Vector3 &a)
{
    Vector3 result{ Uninit };
    result.x = value / a.x;
    result.y = value / a.y;
    result.z = value / a.z;
    return result;
}

static FORCE_INLINE Vector3 minVec(const Vector3 &v1, const Vector3 &v2)
{
    Vector3 result{ Uninit };
    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);
    result.z = fminf(v1.z, v2.z);
    return result;
}

static FORCE_INLINE Vector3 maxVec(const Vector3 &v1, const Vector3 &v2)
{
    Vector3 result{ Uninit };
    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);
    result.z = fmaxf(v1.z, v2.z);
    return result;
}
static FORCE_INLINE float minVec(const Vector3 &v1)
{
    return fminf(v1.z, fminf(v1.x, v1.y));
}

static FORCE_INLINE float maxVec(const Vector3 &v1)
{
    return fmaxf(v1.z, fmaxf(v1.x, v1.y));
}


static FORCE_INLINE float dot(const Vector3 &a, const Vector3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static FORCE_INLINE float sqrLen(const Vector3 &a)
{
    return dot(a, a);
}

static FORCE_INLINE float len(const Vector3 &a)
{
    return fsqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

static FORCE_INLINE Vector3 lerp(const Vector3 &a, const Vector3 &b, float t)
{
    Vec3 result{ Uninit };
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    result.z = a.z + (b.z - a.z) * t;
    return result;
}

static FORCE_INLINE Vector3 normalize(const Vector3 &a)
{
    if(a.x == 0.0f && a.y == 0.0f && a.z == 0.0f)
    {
        ASSERT(false && "Vector3 0 on normalize");
        return Vector3();
    }
    float l = sqrLen(a);
    l = fsqrtf(l);
    float perLen = 1.0f / l;
    return Vector3(a.x * perLen, a.y * perLen, a.z * perLen);
}



static FORCE_INLINE Vector3 cross(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

static FORCE_INLINE Vector3 proj(const Vector3 &a, const Vector3 &b)
{
    return b * (dot(a, b) / dot(b, b));
}

static FORCE_INLINE Vector3 reject(const Vector3 &a, const Vector3 &b)
{
    return a - proj(a, b);
}














static FORCE_INLINE Vector4 operator+(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}
static FORCE_INLINE Vector4 operator+(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x + value;
    result.y = a.y + value;
    result.z = a.z + value;
    result.w = a.w + value;
    return result;
}
static FORCE_INLINE Vector4 operator+(float value, const Vector4 &a)
{
    return a + value;
}


static FORCE_INLINE Vector4 operator-(const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;
    return result;
}


static FORCE_INLINE Vector4 operator-(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

static FORCE_INLINE Vector4 operator-(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x - value;
    result.y = a.y - value;
    result.z = a.z - value;
    result.w = a.w - value;
    return result;
}

static FORCE_INLINE Vector4 operator*(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    result.w = a.w * value;
    return result;
}

static FORCE_INLINE Vector4 operator*(float value, const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    result.w = a.w * value;
    return result;
}

static FORCE_INLINE Vector4 operator*(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}


static FORCE_INLINE Vector4 operator/(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x / value;
    result.y = a.y / value;
    result.z = a.z / value;
    result.w = a.w / value;
    return result;
}

static FORCE_INLINE Vector4 operator/(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    result.w = a.w / b.w;
    return result;
}

static FORCE_INLINE Vector4 operator/(float value, const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = value / a.x;
    result.y = value / a.y;
    result.z = value / a.z;
    result.w = value / a.w;
    return result;
}

static FORCE_INLINE Vector4 minVec(const Vector4 &v1, const Vector4 &v2)
{
    Vector4 result{ Uninit };
    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);
    result.z = fminf(v1.z, v2.z);
    result.w = fminf(v1.w, v2.w);
    return result;
}

static FORCE_INLINE Vector4 maxVec(const Vector4 &v1, const Vector4 &v2)
{
    Vector4 result{ Uninit };
    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);
    result.z = fmaxf(v1.z, v2.z);
    result.w = fmaxf(v1.w, v2.w);
    return result;
}

static FORCE_INLINE float minVec(const Vector4 &v1)
{
    return fminf(fminf(v1.z, v1.w), fminf(v1.x, v1.y));
}

static FORCE_INLINE float maxVec(const Vector4 &v1)
{
    return fmaxf(fmaxf(v1.z, v1.w), fmaxf(v1.x, v1.y));
}

static FORCE_INLINE float dot(const Vector4 &a, const Vector4 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}


static FORCE_INLINE float sqrLen(const Vector4 &a)
{
    return dot(a, a);
}

static FORCE_INLINE float len(const Vector4 &a)
{
    return fsqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

static FORCE_INLINE Vector4 lerp(const Vector4 &a, const Vector4 &b, float t)
{
    Vec4 result{ Uninit };
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    result.z = a.z + (b.z - a.z) * t;
    result.w = a.w + (b.w - a.w) * t;
    return result;
}

static FORCE_INLINE Vector4 normalize(const Vector4 &a)
{
    if(a.x == 0.0f && a.y == 0.0f && a.z == 0.0f && a.w == 0.0f)
    {
        ASSERT(false && "Vector4 0 on normalize");
        return Vector4();
    }
    float l = sqrLen(a);
    l = fsqrtf(l);
    float perLen = 1.0f / l;
    return Vector4(a.x * perLen, a.y * perLen, a.z * perLen, a.w * perLen);
}






