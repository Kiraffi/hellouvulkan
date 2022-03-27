#pragma once

#include "math/vector3.h"
#include "core/mytypes.h"

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

Quaternion getQuaternionFromAxisAngle(const Vector3 &v, float angle);
Quaternion getQuaternionFromNormalizedVectors(const Vector3 &from, const Vector3 &toVector);

Vector3 rotateVector(const Vector3 &v, const Quaternion &q);

void printQuaternion(const Quaternion &q, const char name[]);

using Quat = Quaternion;