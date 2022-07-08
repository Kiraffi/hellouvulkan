#pragma once

#include <math/quaternion.h>
#include <math/vector3_inline_functions.h>


static FORCE_INLINE float dot(const Quaternion &q1, const Quaternion &q2)
{
    return q1.v.x * q2.v.x + q1.v.y * q2.v.y + q1.v.z * q2.v.z + q1.w * q2.w;
}


static FORCE_INLINE Quaternion operator *(const Quaternion &a, const Quaternion &b)
{
    return Quaternion(

        /*
                //right to left, aka right operation first
                a.w * b.v.x + a.v.x * b.w   + a.v.y * b.v.z - a.v.z * b.v.y,
                a.w * b.v.y - a.v.x * b.v.z + a.v.y * b.w   + a.v.z * b.v.x,
                a.w * b.v.z + a.v.x * b.v.y - a.v.y * b.v.x + a.v.z * b.w,
                a.w * b.w   - a.v.x * b.v.x - a.v.y * b.v.y - a.v.z * b.v.z
        */
        // multiply left to right cross(b.v, a.v),
        //cross(b.v, a.v) + a.w * b.v + b.w * a.v, a.w * b.w - dot(a.v, b.v)
        // multiply right to left cross(a.v, b.v)
        cross(a.v, b.v) + a.w * b.v + b.w * a.v, a.w * b.w - dot(a.v, b.v)
    );
}

static FORCE_INLINE Quaternion normalize(const Quaternion &q)
{
    if(q.v.x == 0.0f && q.v.y == 0.0f && q.v.z == 0.0f && q.w == 0.0f)
    {
        ASSERT(false && "Quaternion 0 on normalize");
        return Quaternion();
    }
    float sqrLength = q.v.x * q.v.x + q.v.y * q.v.y + q.v.z * q.v.z + q.w * q.w;
    float length = 1.0f / Supa::sqrtf(sqrLength);
    Quaternion result{ Uninit };
    result.v.x = q.v.x * length;
    result.v.y = q.v.y * length;
    result.v.z = q.v.z * length;
    result.w = q.w * length;
    return result;
}

static FORCE_INLINE Quaternion conjugate(const Quaternion &q)
{
    return Quaternion(-q.v, q.w);
}

static FORCE_INLINE Vector3 rotateVector(const Vector3 &v, const Quaternion &q)
{
    float d = sqrLen(q.v);
    // RH rotation with cross(q.v, v)
    return (v * (q.w * q.w - d) + 2.0f * (q.v * dot(v, q.v) + cross(q.v, v) * q.w));
    // LH rotation cross(v, q.v)
    //return (v * (q.w * q.w - d) + 2.0f * (q.v * dot(v, q.v) + cross(v, q.v) * q.w));
}

static FORCE_INLINE void getAxis(const Quaternion &quat, Vector3 &right, Vector3 &up, Vector3 &forward)
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


static FORCE_INLINE Quaternion getQuaternionFromAxisAngle(const Vector3 &v, float angle)
{
    float s = Supa::sinf(angle * 0.5f);

    Quaternion result;
    result.v = normalize(v) * s;

    result.w = Supa::cosf(angle * 0.5f);
    return result;
}

static FORCE_INLINE Quaternion getQuaternionFromNormalizedVectors(const Vector3 &from, const Vector3 &toVector)
{
    Quaternion result;
    float d = dot(from, toVector);
    if(d >= 1.0f - 1e-5f)
        return result;

    else if(d <= -1.0f + 1e-5f)
    {
        // Generate a rotation axis to do 180 degree rotation
        if(Supa::absf(from.x) < 0.707f)
            return getQuaternionFromAxisAngle(normalize(cross(Vector3(1.0f, 0.0f, 0.0f), from)), PI);
        else
            return getQuaternionFromAxisAngle(normalize(cross(Vector3(0.0f, 1.0f, 0.0f), from)), PI);
    }
    else
    {
        float s = Supa::sqrtf((1.0f + d) * 2.0f);
        result.v = cross(from, toVector);
        result.w = 0.5f * s;
        return normalize(result);
    }
}

static FORCE_INLINE Quaternion operator*(const Quaternion &q, float t)
{
    Quaternion result;
    result.v = q.v * t;
    result.w = q.w * t;
    return result;
}
static FORCE_INLINE Quaternion operator*(float t, const Quaternion &q)
{
    return q * t;
}

static FORCE_INLINE Quaternion lerp(Quaternion const &q1, Quaternion const &q2, float t)
{
    float dotAngle = dot(q1, q2);
    Quaternion result{ Uninit };
    if(dotAngle < 0.0f)
    {
        result.v.x = q1.v.x - t * (q1.v.x + q2.v.x);
        result.v.y = q1.v.y - t * (q1.v.y + q2.v.y);
        result.v.z = q1.v.z - t * (q1.v.z + q2.v.z);
        result.w = q1.w - t * (q1.w + q2.w);
    }
    else
    {
        result.v.x = q1.v.x - t * (q1.v.x - q2.v.x);
        result.v.y = q1.v.y - t * (q1.v.y - q2.v.y);
        result.v.z = q1.v.z - t * (q1.v.z - q2.v.z);
        result.w = q1.w - t * (q1.w - q2.w);
    }
    return result;
}

static FORCE_INLINE Quaternion slerp(Quaternion const &q1, Quaternion const &q2, float t)
{
    float dotAngle = dot(q1, q2);

    Quaternion other = q2;
    if(dotAngle < 0.0f)
    {
        other = Quaternion(-q2.v.x, -q2.v.y, -q2.v.z, -q2.w);//conjugate(q2);
        dotAngle = -dotAngle;
    }

    if(fabsf(dotAngle) > 0.9995)
    {
        Quaternion result;
        result.v = q1.v - t * (q1.v - other.v);
        result.w = q1.w - t * (q1.w - other.w);
        return normalize(result);
    }

    float theta0 = Supa::acosf(dotAngle);
    float theta = theta0 * t;

    float sinTheta = Supa::sinf(theta);
    float sinTheta0 = Supa::sinf(theta0);

    float s2 = sinTheta / sinTheta0;
    float s1 = Supa::cosf(theta) - dotAngle * s2;

    Quaternion result;
    result = q1 * s1;
    result.v = result.v + q2.v * s2;
    result.w += q2.w * s2;
    return normalize(result);
}

static FORCE_INLINE void getDirectionsFromPitchYawRoll(float pitch, float yaw, float roll, Vector3 &rightDir, Vector3 &upDir, Vector3 &forwardDir)
{
    Quat rotation = getQuaternionFromAxisAngle(Vector3(0.0f, 0.0f, 1.0f), roll);
    rotation = getQuaternionFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), pitch) * rotation;
    rotation = getQuaternionFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), yaw) * rotation;

    rightDir = rotateVector(Vector3(1.0f, 0.0, 0.0f), rotation);
    upDir = rotateVector(Vector3(0.0, 1.0, 0.0f), rotation);
    forwardDir = rotateVector(Vector3(0.0, 0.0, 1.0f), rotation);
}


