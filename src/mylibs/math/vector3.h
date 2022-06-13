#pragma once

#include <core/mytypes.h>
#include <core/uninittype.h>

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
float sqrLen(const Vector2 &a);
float len(const Vector2 &a);
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
float sqrLen(const Vector3 &a);
float len(const Vector3 &a);
Vector3 normalize(const Vector3 &a);
float dot(const Vector3 &a, const Vector3 &b);
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
float sqrLen(const Vector4 &a);
float len(const Vector4 &a);
Vector4 normalize(const Vector4 &a);

void printVector3(const Vector3 &v, const char name[]);

void getDirectionsFromPitchYawRoll(float pitch, float yaw, float roll, Vector3& rightDir, Vector3& upDir, Vector3& forwardDir);

using Vec2 = Vector2;
using Vec3 = Vector3;
using Vec4 = Vector4;
