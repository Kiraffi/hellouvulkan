#pragma once

#include <core/mytypes.h>
#include <core/uninittype.h>

#include <math.h>

struct Vector2
{
    Vector2(): x(0.0f), y(0.0f) {}
    Vector2(UninitType) {}
    Vector2(float f) : x(f), y(f) {}

    Vector2(float x, float y) : x(x), y(y) {}
    float x;
    float y;

    float &operator[](uint32_t index) { return (&x)[index]; }
};


struct Vector3
{
    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(UninitType) {}
    Vector3(float f) : x(f), y(f), z(f) {}

    Vector3(const Vector2 &a, float b) : x(a.x), y(a.y), z(b) {}
    Vector3(float b, const Vector2 &a) : x(b), y(a.x), z(a.y) {}

    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    float &operator[](uint32_t index) { return (&x)[index]; }

    float x;
    float y;
    float z;
};

struct Vector4
{
    Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vector4(UninitType) {}
    Vector4(float f) : x(f), y(f), z(f), w(f) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    Vector4(const Vector2 &a, const Vector2 &b) : x(a.x), y(a.y), z(b.x), w(b.y) {}
    Vector4(const Vector2 &a, float b, float c) : x(a.x), y(a.y), z(b), w(c) {}
    Vector4(float b, const Vector2 &a, float c) : x(b), y(a.x), z(a.y), w(c) {}
    Vector4(float b, float c, const Vector2 &a) : x(b), y(c), z(a.x), w(a.y) {}

    Vector4(float b, const Vector3 &a) : x(b), y(a.x), z(a.y), w(a.z) {}
    Vector4(const Vector3 &a, float b) : x(a.x), y(a.y), z(a.z), w(b) {}


    float x;
    float y;
    float z;
    float w;

    float &operator[](uint32_t index) { return ( &x )[ index ]; }
};

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


void printVector3(const Vector3 &v, const char *name);

using Vec2 = Vector2;
using Vec3 = Vector3;
using Vec4 = Vector4;







static Vector2 operator+(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

static Vector2 operator+(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x + value;
    result.y = a.y + value;
    return result;
}

static Vector2 operator+(float value, const Vector2 &a)
{
    return a + value;
}


static Vector2 operator-(const Vector2 &a)
{
    Vector2 result{ Uninit };
    result.x = -a.x;
    result.y = -a.y;
    return result;
}


static Vector2 operator-(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

static Vector2 operator-(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x - value;
    result.y = a.y - value;
    return result;
}

static Vector2 operator*(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    return result;
}

static Vector2 operator*(float value, const Vector2 &a)
{
    Vector2 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    return result;
}

static Vector2 operator*(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

static Vector2 operator/(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x / value;
    result.y = a.y / value;
    return result;
}

static Vector2 operator/(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

static Vector2 operator/(float value, const Vector2 &a)
{
    Vector2 result{ Uninit };
    result.x = value / a.x;
    result.y = value / a.y;
    return result;
}


static Vector2 min(const Vector2 &v1, const Vector2 &v2)
{
    Vector2 result{ Uninit };
    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);
    return result;
}

static Vector2 max(const Vector2 &v1, const Vector2 &v2)
{
    Vector2 result{ Uninit };
    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);
    return result;
}

static float min(const Vector2 &v1)
{
    return fminf(v1.x, v1.y);
}

static float max(const Vector2 &v1)
{
    return fmaxf(v1.x, v1.y);
}


static float dot(const Vector2 &a, const Vector2 &b)
{
    return a.x * b.x + a.y * b.y;
}

static float sqrLen(const Vector2 &a)
{
    return dot(a, a);
}

static float len(const Vector2 &a)
{
    return fsqrtf(a.x * a.x + a.y * a.y);
}

static Vector2 lerp(const Vector2 &a, const Vector2 &b, float t)
{
    Vec2 result{ Uninit };
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    return result;
}


static Vector2 normalize(const Vector2 &a)
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










static Vector3 operator+(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}
static Vector3 operator+(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x + value;
    result.y = a.y + value;
    result.z = a.z + value;
    return result;
}
static Vector3 operator+(float value, const Vector3 &a)
{
    return a + value;
}


static Vector3 operator-(const Vector3 &a)
{
    Vector3 result{ Uninit };
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    return result;
}


static Vector3 operator-(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

static Vector3 operator-(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x - value;
    result.y = a.y - value;
    result.z = a.z - value;
    return result;
}

static Vector3 operator*(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    return result;
}

static Vector3 operator*(float value, const Vector3 &a)
{
    Vector3 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    return result;
}

static Vector3 operator*(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}


static Vector3 operator/(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x / value;
    result.y = a.y / value;
    result.z = a.z / value;
    return result;
}

static Vector3 operator/(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

static Vector3 operator/(float value, const Vector3 &a)
{
    Vector3 result{ Uninit };
    result.x = value / a.x;
    result.y = value / a.y;
    result.z = value / a.z;
    return result;
}

static Vector3 min(const Vector3 &v1, const Vector3 &v2)
{
    Vector3 result{ Uninit };
    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);
    result.z = fminf(v1.z, v2.z);
    return result;
}

static Vector3 max(const Vector3 &v1, const Vector3 &v2)
{
    Vector3 result{ Uninit };
    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);
    result.z = fmaxf(v1.z, v2.z);
    return result;
}
static float min(const Vector3 &v1)
{
    return fminf(v1.z, fminf(v1.x, v1.y));
}

static float max(const Vector3 &v1)
{
    return fmaxf(v1.z, fmaxf(v1.x, v1.y));
}


static float dot(const Vector3 &a, const Vector3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float sqrLen(const Vector3 &a)
{
    return dot(a, a);
}

static float len(const Vector3 &a)
{
    return fsqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

static Vector3 lerp(const Vector3 &a, const Vector3 &b, float t)
{
    Vec3 result{ Uninit };
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    result.z = a.z + (b.z - a.z) * t;
    return result;
}

static Vector3 normalize(const Vector3 &a)
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



static Vector3 cross(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

static Vector3 proj(const Vector3 &a, const Vector3 &b)
{
    return b * (dot(a, b) / dot(b, b));
}

static Vector3 reject(const Vector3 &a, const Vector3 &b)
{
    return a - proj(a, b);
}














static Vector4 operator+(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}
static Vector4 operator+(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x + value;
    result.y = a.y + value;
    result.z = a.z + value;
    result.w = a.w + value;
    return result;
}
static Vector4 operator+(float value, const Vector4 &a)
{
    return a + value;
}


static Vector4 operator-(const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;
    return result;
}


static Vector4 operator-(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

static Vector4 operator-(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x - value;
    result.y = a.y - value;
    result.z = a.z - value;
    result.w = a.w - value;
    return result;
}

static Vector4 operator*(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    result.w = a.w * value;
    return result;
}

static Vector4 operator*(float value, const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    result.w = a.w * value;
    return result;
}

static Vector4 operator*(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}


static Vector4 operator/(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x / value;
    result.y = a.y / value;
    result.z = a.z / value;
    result.w = a.w / value;
    return result;
}

static Vector4 operator/(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    result.w = a.w / b.w;
    return result;
}

static Vector4 operator/(float value, const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = value / a.x;
    result.y = value / a.y;
    result.z = value / a.z;
    result.w = value / a.w;
    return result;
}

static Vector4 min(const Vector4 &v1, const Vector4 &v2)
{
    Vector4 result{ Uninit };
    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);
    result.z = fminf(v1.z, v2.z);
    result.w = fminf(v1.w, v2.w);
    return result;
}

static Vector4 max(const Vector4 &v1, const Vector4 &v2)
{
    Vector4 result{ Uninit };
    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);
    result.z = fmaxf(v1.z, v2.z);
    result.w = fmaxf(v1.w, v2.w);
    return result;
}

static float min(const Vector4 &v1)
{
    return fminf(fminf(v1.z, v1.w), fminf(v1.x, v1.y));
}

static float max(const Vector4 &v1)
{
    return fmaxf(fmaxf(v1.z, v1.w), fmaxf(v1.x, v1.y));
}

static float dot(const Vector4 &a, const Vector4 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}


static float sqrLen(const Vector4 &a)
{
    return dot(a, a);
}

static float len(const Vector4 &a)
{
    return fsqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

static Vector4 lerp(const Vector4 &a, const Vector4 &b, float t)
{
    Vec4 result{ Uninit };
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    result.z = a.z + (b.z - a.z) * t;
    result.w = a.w + (b.w - a.w) * t;
    return result;
}

static Vector4 normalize(const Vector4 &a)
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






