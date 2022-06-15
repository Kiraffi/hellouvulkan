#pragma once

#include <components/transform.h>

#include <core/uninittype.h>
#include <math/bounds.h>
#include <math/sphere.h>
#include <math/vector3.h>

struct Ray
{
    Ray() : pos(0.0f, 0.0f, 0.0f), dir(0.0f, 0.0f, 1.0f) {}
    Ray(const Vec3 &pos, const Vec3 &dir) : pos(pos), dir(dir) {}
    Ray(UninitType) {}

    Vec3 pos;
    Vec3 dir; // assuming normalized
};

struct Hitpoint
{
    Hitpoint() : point(0.0f, 0.0f, 0.0f), distance(0.0f) {}
    Hitpoint(UninitType) {}

    Vec3 point;
    float distance;
};

bool raySphereIntersect(const Ray &ray, const Sphere &sphere, Hitpoint &outHitpoint);

bool rayOOBBBoundsIntersect(const Ray &ray, const Bounds &bounds, const Transform &transform, Hitpoint &outHitpoint);