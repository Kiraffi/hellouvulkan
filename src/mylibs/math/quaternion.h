#pragma once

#include <math/vector3.h>
#include <core/mytypes.h>

struct Quaternion;

Quaternion normalize(const Quaternion &q);
void getAxis(const Quaternion &quat, Vector3 &right, Vector3 &up, Vector3 &forward);
Quaternion operator *(const Quaternion &a, const Quaternion &b);

struct Quaternion
{
    Quaternion() {}
    Quaternion(const Vector3 &v, float w) : v(v), w(w) {}
    Quaternion(float x, float y, float z, float w) : v(x, y, z), w(w) {}
    Vector3 v;
    float w = 1.0f;



};

Quaternion operator*(const Quaternion &q, float t);
Quaternion operator*(float t, const Quaternion &q);

Quaternion getQuaternionFromAxisAngle(const Vector3 &v, float angle);
Quaternion getQuaternionFromNormalizedVectors(const Vector3 &from, const Vector3 &toVector);


Vector3 rotateVector(const Vector3 &v, const Quaternion &q);

float dot(const Quaternion &q1, const Quaternion &q2);
Quaternion slerp(Quaternion const &q1, Quaternion const &q2, float t);
Quaternion lerp(Quaternion const& q1, Quaternion const& q2, float t);
void printQuaternion(const Quaternion &q, const char name[]);

using Quat = Quaternion;