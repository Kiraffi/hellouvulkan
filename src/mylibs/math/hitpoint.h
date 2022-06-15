#pragma once

#include <math/vector3.h>

struct HitPoint
{
    HitPoint() : point(0.0f, 0.0f, 0.0f), distance(0.0f) {}
    HitPoint(UninitType) {}

    Vec3 point;
    float distance;
};
