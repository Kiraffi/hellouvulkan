#include "vector3.h"

Vector2 operator+(const Vector2 &a, const Vector2 &b)
{
	Vector2 result(a);
	result.x += b.x;
	result.y += b.y;
	return result;
}

Vector2 operator-(const Vector2 &a)
{
	Vector2 result;
	result.x = -a.x;
	result.y = -a.y;
	return result;
}


Vector2 operator-(const Vector2 &a, const Vector2 &b)
{
	Vector2 result(a);
	result.x -= b.x;
	result.y -= b.y;
	return result;
}

Vector2 operator*(const Vector2 &a, float value)
{
	Vector2 result(a);
	result.x *= value;
	result.y *= value;
	return result;
}

Vector2 operator*(float value, const Vector2 &a)
{
	Vector2 result(a);
	result.x *= value;
	result.y *= value;
	return result;
}


float sqrLen(const Vector2 &a)
{
	return (a.x * a.x + a.y * a.y);
}

float len(const Vector2 &a)
{
	return fsqrtf(a.x * a.x + a.y * a.y);
}

Vector2 normalize(const Vector2 &a)
{
	float l = len(a);
	ASSERT(l != 0.0f);
	float perLen = 1.0f / l;
	Vector2 result(a.x * perLen, a.y * perLen);

	return result;
}












Vector3 operator+(const Vector3 &a, const Vector3 &b)
{
	Vector3 result(a);
	result.x += b.x;
	result.y += b.y;
	result.z += b.z;
	return result;
}

Vector3 operator-(const Vector3 &a)
{
	Vector3 result;
	result.x = -a.x;
	result.y = -a.y;
	result.z = -a.z;
	return result;
}


Vector3 operator-(const Vector3 &a, const Vector3 &b)
{
	Vector3 result(a);
	result.x -= b.x;
	result.y -= b.y;
	result.z -= b.z;
	return result;
}

Vector3 operator*(const Vector3 &a, float value)
{
	Vector3 result(a);
	result.x *= value;
	result.y *= value;
	result.z *= value;
	return result;
}

Vector3 operator*(float value, const Vector3 &a)
{
	Vector3 result(a);
	result.x *= value;
	result.y *= value;
	result.z *= value;
	return result;
}


float sqrLen(const Vector3 &a)
{
	return (a.x * a.x + a.y * a.y + a.z * a.z);
}

float len(const Vector3 &a)
{
	return fsqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

Vector3 normalize(const Vector3 &a)
{
	float l = len(a);
	ASSERT(l != 0.0f);
	float perLen = 1.0f / l;
	Vector3 result(a.x * perLen, a.y * perLen, a.z * perLen);

	return result;
}


float dot(const Vector3 &a, const Vector3 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 cross(const Vector3 &a, const Vector3 &b)
{
	Vector3 result;
	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
	return result;
}

Vector3 proj(const Vector3 &a, const Vector3 &b)
{
	return b * (dot(a, b) / dot(b, b));
}

Vector3 reject(const Vector3 &a, const Vector3 &b)
{
	return a - proj(a, b);
}

void printVector3(const Vector3 &v, const char name[])
{
	LOG("%s: %f, %f, %f\n", name, v.x, v.y, v.z);
}
