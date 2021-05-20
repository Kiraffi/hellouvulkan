#pragma once

#include <core/mytypes.h>

struct Vector2
{
	Vector2() {}
	Vector2(float x, float y) : x(x), y(y) {}
	float x = 0.0f;
	float y = 0.0f;

	float &operator[](u32 index) { return (&x)[index]; }
};


struct Vector3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	Vector3() {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

	float &operator[](u32 index) { return (&x)[index]; }

};
struct Vector4
{
	Vector4() {}
	Vector4(float x, float y, float z, float w): x(x), y(y), z(z), w(w) {}
	Vector4(float f): x(f), y(f), z(f), w(f) {}
	Vector4(const Vector3 &v, float w): x(v.x), y(v.y), z(v.z), w(w) {}

	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float w = 0.0f;

	float &operator[](u32 index) { return ( &x )[ index ]; }
};

Vector2 operator+(const Vector2 &a, const Vector2 &b);
Vector2 operator-(const Vector2 &a);
Vector2 operator-(const Vector2 &a, const Vector2 &b);
Vector2 operator*(const Vector2 &a, float value);
Vector2 operator*(float value, const Vector2 &a);
float sqrLen(const Vector2 &a);
float len(const Vector2 &a);
Vector2 normalize(const Vector2 &a);


Vector3 operator+(const Vector3 &a, const Vector3 &b);
Vector3 operator-(const Vector3 &a);
Vector3 operator-(const Vector3 &a, const Vector3 &b);
Vector3 operator*(const Vector3 &a, float value);
Vector3 operator*(float value, const Vector3 &a);
float sqrLen(const Vector3 &a);
float len(const Vector3 &a);
Vector3 normalize(const Vector3 &a);
float dot(const Vector3 &a, const Vector3 &b);
Vector3 cross(const Vector3 &a, const Vector3 &b);
Vector3 proj(const Vector3 &a, const Vector3 &b);
Vector3 reject(const Vector3 &a, const Vector3 &b);

void printVector3(const Vector3 &v, const char name[]);


using Vec2 = Vector2;
using Vec3 = Vector3;
using Vec4 = Vector4;
