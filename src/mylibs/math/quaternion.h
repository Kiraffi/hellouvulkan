#pragma once

#include <core/mytypes.h>
#include <core/uninittype.h>
#include <math/vector3.h>


struct Quaternion
{
    Quaternion() : v(0.0f, 0.0f, 0.0f), w(1.0f) {}
    Quaternion(UninitType) : v{ Uninit } {}
    Quaternion(const Vector3 &v, float w) : v(v), w(w) {}
    Quaternion(float x, float y, float z, float w) : v(x, y, z), w(w) {}
    Vector3 v;
    float w;
};

void printQuaternion(const Quaternion &q, const char *name);

using Quat = Quaternion;

