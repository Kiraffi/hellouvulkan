#include "ray.h"

#include <math/bounds.h>
#include <math/hitpoint.h>
#include <math/quaternion_inline_functions.h>
#include <math/vector3_inline_functions.h>

// intersect test ray sphere from game physics cookbook
bool raySphereIntersect(const Ray &ray, const Sphere &sphere, HitPoint &outHitpoint)
{
    float sphereRadius = 1.0f;
    Vec3 e = sphere.pos - ray.pos;
    float rSq = sphere.radius * sphere.radius;

    float eSq = sqrLen(e);
    float a = dot(e, ray.dir);
    float bSq = eSq - (a * a);
    float f = Supa::sqrtf(Supa::absf((rSq)-bSq));

    // Assume normal intersection!
    float t = a - f;

    // No collision has happened
    if(rSq - bSq < 0.0f) {
        return false;
    }
    // Ray starts inside the sphere
    else if(eSq < rSq) {
        // Just reverse direction
        t = a + f;
    }

    outHitpoint.point = ray.pos + ray.dir * t;
    outHitpoint.distance = t;

    return true;
}



bool rayOOBBBoundsIntersect(const Ray &ray, const Bounds &bounds, const Transform &transform, HitPoint &outHitpoint)
{
    const Quaternion inverseQuat = conjugate(transform.rot);
    const Vec3 rayPos = rotateVector(ray.pos - transform.pos, inverseQuat);
    const Vec3 rayDir = rotateVector(ray.dir, inverseQuat);
    const Vec3 inverseRayDir = 1.0f / rayDir;

    Vec3 tMin = ((bounds.min * transform.scale) - rayPos) * inverseRayDir;
    Vec3 tMax = ((bounds.max * transform.scale) - rayPos) * inverseRayDir;

    Vec3 tMins = Vec3(minVec(tMin, tMax));
    Vec3 tMaxs = Vec3(maxVec(tMin, tMax));

    float tMind = fmaxf(fmaxf(tMins.x, tMins.y), tMins.z);
    float tMaxd = fminf(fminf(tMaxs.x, tMaxs.y), tMaxs.z);

    // can have negative tMinD, then we are inside, if both are negative then the hit is behind
    outHitpoint.distance = tMind >= 0.0f ? tMind : 0.0f;
    outHitpoint.point = outHitpoint.distance * ray.dir + ray.pos;
    bool hit = tMaxd >= tMind && tMaxd >= 0.0f;

    return hit;
}