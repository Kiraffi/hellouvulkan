#include "vector3.h"
#include <math/quaternion.h>

#include <cmath>

Vector2 operator+(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

Vector2 operator+(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x + value;
    result.y = a.y + value;
    return result;
}

Vector2 operator+(float value, const Vector2 &a)
{
    return a + value;
}


Vector2 operator-(const Vector2 &a)
{
    Vector2 result{ Uninit };
    result.x = -a.x;
    result.y = -a.y;
    return result;
}


Vector2 operator-(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

Vector2 operator-(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x - value;
    result.y = a.y - value;
    return result;
}

Vector2 operator*(const Vector2 &a, float value)
{
    Vector2 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    return result;
}

Vector2 operator*(float value, const Vector2 &a)
{
    Vector2 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    return result;
}

Vector2 operator*(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

Vector2 operator/(const Vector2 &a,float value)
{
    Vector2 result{ Uninit };
    result.x = a.x / value;
    result.y = a.y / value;
    return result;
}

Vector2 operator/(const Vector2 &a, const Vector2 &b)
{
    Vector2 result{ Uninit };
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

Vector2 operator/(float value, const Vector2 &a)
{
    Vector2 result{ Uninit };
    result.x = value / a.x;
    result.y = value / a.y;
    return result;
}


Vector2 min(const Vector2 &v1, const Vector2 &v2)
{
    Vector2 result{ Uninit };
    result.x = std::min(v1.x, v2.x);
    result.y = std::min(v1.y, v2.y);
    return result;
}

Vector2 max(const Vector2 &v1, const Vector2 &v2)
{
    Vector2 result{ Uninit };
    result.x = std::max(v1.x, v2.x);
    result.y = std::max(v1.y, v2.y);
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
    float l = sqrLen(a);
    ASSERT(l != 0.0f);
    if(l > 0.0f)
    {
        l = fsqrtf(l);
        float perLen = 1.0f / l;
        return Vector2(a.x * perLen, a.y * perLen);
    }
    return a;
}












Vector3 operator+(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}
Vector3 operator+(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x + value;
    result.y = a.y + value;
    result.z = a.z + value;
    return result;
}
Vector3 operator+(float value, const Vector3 &a)
{
    return a + value;
}


Vector3 operator-(const Vector3 &a)
{
    Vector3 result{ Uninit };
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    return result;
}


Vector3 operator-(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

Vector3 operator-(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x - value;
    result.y = a.y - value;
    result.z = a.z - value;
    return result;
}

Vector3 operator*(const Vector3 &a, float value)
{
    Vector3 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    return result;
}

Vector3 operator*(float value, const Vector3 &a)
{
    Vector3 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    return result;
}

Vector3 operator*(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}


Vector3 operator/(const Vector3 &a,float value)
{
    Vector3 result{ Uninit };
    result.x = a.x / value;
    result.y = a.y / value;
    result.z = a.z / value;
    return result;
}

Vector3 operator/(const Vector3 &a, const Vector3 &b)
{
    Vector3 result{ Uninit };
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

Vector3 operator/(float value, const Vector3 &a)
{
    Vector3 result{ Uninit };
    result.x = value / a.x;
    result.y = value / a.y;
    result.z = value / a.z;
    return result;
}

Vector3 min(const Vector3 &v1, const Vector3 &v2)
{
    Vector3 result{ Uninit };
    result.x = std::min(v1.x, v2.x);
    result.y = std::min(v1.y, v2.y);
    result.z = std::min(v1.z, v2.z);
    return result;
}

Vector3 max(const Vector3 &v1, const Vector3 &v2)
{
    Vector3 result{ Uninit };
    result.x = std::max(v1.x, v2.x);
    result.y = std::max(v1.y, v2.y);
    result.z = std::max(v1.z, v2.z);
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
    float l = sqrLen(a);
    ASSERT(l > 0.0f);
    if(l > 0.0f)
    {
        l = fsqrtf(l);
        float perLen = 1.0f / l;
        return Vector3(a.x * perLen, a.y * perLen, a.z * perLen);
    }
    return a;
}


float dot(const Vector3 &a, const Vector3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 cross(const Vector3 &a, const Vector3 &b)
{
    Vector3 result {Uninit};
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














Vector4 operator+(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.w + b.z;
    result.w = a.z + b.w;
    return result;
}
Vector4 operator+(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x + value;
    result.y = a.y + value;
    result.z = a.w + value;
    result.w = a.z + value;
    return result;
}
Vector4 operator+(float value, const Vector4 &a)
{
    return a + value;
}


Vector4 operator-(const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.w;
    result.w = -a.z;
    return result;
}


Vector4 operator-(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.w - b.z;
    result.w = a.z - b.w;
    return result;
}

Vector4 operator-(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x - value;
    result.y = a.y - value;
    result.z = a.w - value;
    result.w = a.z - value;
    return result;
}

Vector4 operator*(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.w * value;
    result.w = a.z * value;
    return result;
}

Vector4 operator*(float value, const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.w * value;
    result.w = a.z * value;
    return result;
}

Vector4 operator*(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.w * b.z;
    result.w = a.z * b.w;
    return result;
}


Vector4 operator/(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x / value;
    result.y = a.y / value;
    result.z = a.w / value;
    result.w = a.z / value;
    return result;
}

Vector4 operator/(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.w / b.z;
    result.w = a.z / b.w;
    return result;
}

Vector4 operator/(float value, const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = value / a.x;
    result.y = value / a.y;
    result.z = value / a.w;
    result.w = value / a.z;
    return result;
}

Vector4 min(const Vector4 &v1, const Vector4 &v2)
{
    Vector4 result{ Uninit };
    result.x = std::min(v1.x, v2.x);
    result.y = std::min(v1.y, v2.y);
    result.z = std::min(v1.z, v2.z);
    result.w = std::min(v1.w, v2.w);
    return result;
}

Vector4 max(const Vector4 &v1, const Vector4 &v2)
{
    Vector4 result{ Uninit };
    result.x = std::max(v1.x, v2.x);
    result.y = std::max(v1.y, v2.y);
    result.z = std::max(v1.z, v2.z);
    result.w = std::max(v1.w, v2.w);
    return result;
}


float sqrLen(const Vector4 &a)
{
    return (a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

float len(const Vector4 &a)
{
    return fsqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

Vector4 normalize(const Vector4 &a)
{
    float l = sqrLen(a);
    ASSERT(l > 0.0f);
    if(l > 0.0f)
    {
        l = fsqrtf(l);
        float perLen = 1.0f / l;
        return a * perLen;
    }
    return a;
}














void printVector3(const Vector3 &v, const char name[])
{
    LOG("%s: %f, %f, %f\n", name, v.x, v.y, v.z);
}

void getDirectionsFromPitchYawRoll(float pitch, float yaw, float roll, Vector3& rightDir, Vector3& upDir, Vector3& forwardDir)
{
    Quat rotation = getQuaternionFromAxisAngle(Vector3(0.0f, 0.0f, 1.0f), roll);
    rotation = getQuaternionFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), pitch) * rotation;
    rotation = getQuaternionFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), yaw) * rotation;

    rightDir = rotateVector(Vector3(1.0f, 0.0, 0.0f), rotation);
    upDir = rotateVector(Vector3(0.0, 1.0, 0.0f), rotation);
    forwardDir = rotateVector(Vector3(0.0, 0.0, 1.0f), rotation);
}