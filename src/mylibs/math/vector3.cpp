#include "vector3.h"
#include <math/quaternion.h>

#include <math.h>

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
    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);
    return result;
}

Vector2 max(const Vector2 &v1, const Vector2 &v2)
{
    Vector2 result{ Uninit };
    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);
    return result;
}

float min(const Vector2 &v1)
{
    return fminf(v1.x, v1.y);
}

float max(const Vector2 &v1)
{
    return fmaxf(v1.x, v1.y);
}


float sqrLen(const Vector2 &a)
{
    return dot(a, a);
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

float dot(const Vector2 &a, const Vector2 &b)
{
    return a.x * b.x + a.y * b.y;
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
    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);
    result.z = fminf(v1.z, v2.z);
    return result;
}

Vector3 max(const Vector3 &v1, const Vector3 &v2)
{
    Vector3 result{ Uninit };
    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);
    result.z = fmaxf(v1.z, v2.z);
    return result;
}
float min(const Vector3 &v1)
{
    return fminf(v1.z, fminf(v1.x, v1.y));
}

float max(const Vector3 &v1)
{
    return fmaxf(v1.z, fmaxf(v1.x, v1.y));
}


float sqrLen(const Vector3 &a)
{
    return dot(a, a);
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
        return a * perLen;
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
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}
Vector4 operator+(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x + value;
    result.y = a.y + value;
    result.z = a.z + value;
    result.w = a.w + value;
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
    result.z = -a.z;
    result.w = -a.w;
    return result;
}


Vector4 operator-(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

Vector4 operator-(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x - value;
    result.y = a.y - value;
    result.z = a.z - value;
    result.w = a.w - value;
    return result;
}

Vector4 operator*(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    result.w = a.w * value;
    return result;
}

Vector4 operator*(float value, const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = a.x * value;
    result.y = a.y * value;
    result.z = a.z * value;
    result.w = a.w * value;
    return result;
}

Vector4 operator*(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}


Vector4 operator/(const Vector4 &a, float value)
{
    Vector4 result{ Uninit };
    result.x = a.x / value;
    result.y = a.y / value;
    result.z = a.z / value;
    result.w = a.w / value;
    return result;
}

Vector4 operator/(const Vector4 &a, const Vector4 &b)
{
    Vector4 result{ Uninit };
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    result.w = a.w / b.w;
    return result;
}

Vector4 operator/(float value, const Vector4 &a)
{
    Vector4 result{ Uninit };
    result.x = value / a.x;
    result.y = value / a.y;
    result.z = value / a.z;
    result.w = value / a.w;
    return result;
}

Vector4 min(const Vector4 &v1, const Vector4 &v2)
{
    Vector4 result{ Uninit };
    result.x = fminf(v1.x, v2.x);
    result.y = fminf(v1.y, v2.y);
    result.z = fminf(v1.z, v2.z);
    result.w = fminf(v1.w, v2.w);
    return result;
}

Vector4 max(const Vector4 &v1, const Vector4 &v2)
{
    Vector4 result{ Uninit };
    result.x = fmaxf(v1.x, v2.x);
    result.y = fmaxf(v1.y, v2.y);
    result.z = fmaxf(v1.z, v2.z);
    result.w = fmaxf(v1.w, v2.w);
    return result;
}

float min(const Vector4 &v1)
{
    return fminf(fminf(v1.z, v1.w), fminf(v1.x, v1.y));
}

float max(const Vector4 &v1)
{
    return fmaxf(fmaxf(v1.z, v1.w), fmaxf(v1.x, v1.y));
}


float sqrLen(const Vector4 &a)
{
    return dot(a, a);
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

float dot(const Vector4 &a, const Vector4 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
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