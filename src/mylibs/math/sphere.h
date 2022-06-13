#pragma once

#include <core/uninittype.h>
#include <math/vector3.h>

struct Sphere
{
    Sphere() : pos(0.0f, 0.0f, 0.0f), radius(0.0f) {}
    Sphere(UninitType) {}
    Vec3 pos;
    float radius;
};

