#pragma once

#include <core/transform.h>
#include <math/bounds.h>
#include <math/sphere.h>
#include <math/vector3.h>

struct Ray
{
    Vec3 pos;
    // assuming normalized
    Vec3 dir{ 0.0f, 0.0f, 1.0f };
};

struct Hitpoint
{
    Vec3 point;
    float distance;
};

bool raySphereIntersect(const Ray &ray, const Sphere &sphere, Hitpoint &outHitpoint);

bool rayOOBBBoundsIntersect(const Ray &ray, const Bounds &bounds, const Transform &transform, Hitpoint &outHitpoint);