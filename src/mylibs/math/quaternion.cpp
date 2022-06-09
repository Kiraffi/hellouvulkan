#include "quaternion.h"
//#include <algorithm>
#include <cmath>
//#include <stdio.h>


Quaternion operator *(const Quaternion &a, const Quaternion &b)
{
    return Quaternion(
        /*
        a.w * b.v.x + a.v.x * b.w   + a.v.y * b.v.z - a.v.z * b.v.y,
        a.w * b.v.y - a.v.x * b.v.z + a.v.y * b.w   + a.v.z * b.v.x,
        a.w * b.v.z + a.v.x * b.v.y - a.v.y * b.v.x + a.v.z * b.w,
        a.w * b.w   - a.v.x * b.v.x - a.v.y * b.v.y - a.v.z * b.v.z
        */
        cross(a.v, b.v) + a.w * b.v + b.w * a.v, a.w * b.w - dot(a.v, b.v)
    );
}

Quaternion normalize(const Quaternion &q)
{
    Quaternion result;
    
    float length = std::sqrtf(dot(q, q)); // std::sqrtf(sqrLen(q.v) + result.w * result.w));
    if(length > 0)
    {
        result.w = q.w / length;
        result.v = q.v / length;
    }
    /*
    result.w = std::max(std::min(q.w, 1.0f), -1.0f);
    if(result.w < 1.0f && result.w > -1.0f)
        result.v = normalize(q.v) * fsqrtf(1.0f - result.w * result.w);
        */
    return result;
}

Quaternion conjugate(const Quaternion &q)
{
    return Quaternion(-q.v, q.w);
}

Vector3 rotateVector(const Vector3 &v, const Quaternion &q)
{

    float d = sqrLen(q.v);
    return (v * (q.w * q.w - d) + 2.0f * (q.v * dot(v, q.v) + cross(v, q.v) * q.w));
}

void getAxis(const Quaternion &quat, Vector3 &right, Vector3 &up, Vector3 &forward)
{
    right.x = 1.0f - 2.0f * quat.v.y * quat.v.y - 2.0f * quat.v.z * quat.v.z;
    right.y = 2.0f * quat.v.x * quat.v.y + 2.0f * quat.w * quat.v.z;
    right.z = 2.0f * quat.v.x * quat.v.z - 2.0f * quat.w * quat.v.y;

    up.x = 2.0f * quat.v.x * quat.v.y - 2.0f * quat.w * quat.v.z;
    up.y = 1.0f - 2.0f * quat.v.x * quat.v.x - 2.0f * quat.v.z * quat.v.z;
    up.z = 2.0f * quat.v.y * quat.v.z + 2.0f * quat.w * quat.v.x;

    forward.x = 2.0f * quat.v.x * quat.v.z + 2.0f * quat.w * quat.v.y;
    forward.y = 2.0f * quat.v.y * quat.v.z - 2.0f * quat.w * quat.v.x;
    forward.z = 1.0f - 2.0f * quat.v.x * quat.v.x - 2.0f * quat.v.y * quat.v.y;

}


Quaternion getQuaternionFromAxisAngle(const Vector3 &v, float angle)
{
    float s = fsinf(angle * 0.5f);

    Quaternion result;
    result.v = normalize(v) * s;

    result.w = fcosf(angle * 0.5f);
    return result;
}

Quaternion getQuaternionFromNormalizedVectors(const Vector3 &from, const Vector3 &toVector)
{
    Quaternion result;
    float d = dot(from, toVector);
    if (d >= 1.0f - 1e-5f)
        return result;

    else if (d <= -1.0f + 1e-5f)
    {
        // Generate a rotation axis to do 180 degree rotation
        if (ffabsf(from.x) < 0.707f)
            return getQuaternionFromAxisAngle(normalize(cross(Vector3(1.0f, 0.0f, 0.0f), from)), PI);
        else
            return getQuaternionFromAxisAngle(normalize(cross(Vector3(0.0f, 1.0f, 0.0f), from)), PI);
    }
    else
    {
        float s = fsqrtf((1.0f + d) * 2.0f);
        result.v = cross(from, toVector);
        result.w = 0.5f * s;
        return normalize(result);
    }
}
float dot(const Quaternion &q1, const Quaternion &q2)
{
    return q1.v.x * q2.v.x + q1.v.y * q2.v.y + q1.v.z * q2.v.z + q1.w * q2.w;
}

Quaternion operator*(const Quaternion &q, float t)
{
    Quaternion result;
    result.v = q.v * t;
    result.w = q.w * t;
    return result;
}
Quaternion operator*(float t, const Quaternion &q)
{
    return q * t;
}

Quaternion lerp(Quaternion const& q1, Quaternion const& q2, float t)
{
    float dotAngle = dot(q1, q2);
    Quaternion other = q2;
    if(dotAngle < 0.0f)
        other = Quaternion(-q2.v.x, -q2.v.y, -q2.v.z, -q2.w);//conjugate(q2);

    Quaternion result;
    result.v = q1.v - t * (q1.v - other.v);
    result.w = q1.w - t * (q1.w - other.w);
    return result;
}

Quaternion slerp(Quaternion const &q1, Quaternion const &q2, float t)
{

    float dotAngle = dot(q1, q2);

    Quaternion other = q2;
    if (dotAngle < 0.0f)
    {
        other = Quaternion(-q2.v.x, -q2.v.y, -q2.v.z, -q2.w);//conjugate(q2);
        dotAngle = -dotAngle;
    }
    
    if (std::abs(dotAngle) > 0.9995)
    {
        Quaternion result;
        result.v = q1.v - t * (q1.v - other.v);
        result.w = q1.w - t * (q1.w - other.w);
        return normalize(result);
    }
    
    float theta0 = std::acos(dotAngle);
    float theta = theta0 * t;

    float sinTheta = sin(theta);
    float sinTheta0 = sin(theta0);

    float s2 = sinTheta / sinTheta0;
    float s1 = cos(theta) - dotAngle * s2;

    Quaternion result;
    result = q1 * s1;
    result.v = result.v + q2.v * s2;
    result.w += q2.w * s2;
    return normalize(result);
}

void printQuaternion(const Quaternion &q, const char *name)
{
    LOG("%s: %f, %f, %f, %f\n", name, q.v.x, q.v.y, q.v.z, q.w);
}
