#include "testfuncs.h"

#include <core/mytypes.h>
#include <math/vector3_inline_functions.h>

#include <math.h>


static float VecEpsilon = 1.0e-5f;

void testVec2()
{
    const Vec2 v(2.0f, -3.0f);
    const Vec2 v2(5.0f, 7.0f);
    {
        ASSERT(v.x == 2.0f && v.y == -3.0f);
    }

    {
        const Vec2 res1 = -v;
        ASSERT(res1.x == -2.0f && res1.y == 3.0f);

        const Vec2 res2 = v - v2;
        ASSERT(res2.x == -3.0f && res2.y == -10.0f);

        const Vec2 res3 = v - 11.0f;
        ASSERT(res3.x == -9.0f && res3.y == -14.0f);
    }

    {
        const Vec2 res1 = v + v2;
        ASSERT(res1.x == 7.0f && res1.y == 4.0f);

        const Vec2 res2 = v + 11.0f;
        ASSERT(res2.x == 13.0f && res2.y == 8.0f);

        const Vec2 res3 = 13.0f + v;
        ASSERT(res3.x == 15.0f && res3.y == 10.0f);
    }
    {
        const Vec2 res1 = v * v2;
        ASSERT(res1.x == 10.0f && res1.y == -21.0f);

        const Vec2 res2 = v * 13.0f;
        ASSERT(res2.x == 26.0f && res2.y == -39.0f);

        const Vec2 res3 = 17.0f * v;
        ASSERT(res3.x == 34.0f && res3.y == -51.0f);
    }

    {
        const Vec2 vec1(9.0f, -20.0f);
        const Vec2 vec2(3.0f, 4.0f);

        const Vec2 res1 = vec1 / vec2;
        ASSERT(res1.x == 3.0f && res1.y == -5.0f);

        const Vec2 res2 = vec1 / 8.0f;
        ASSERT(res2.x == 9.0f / 8.0f && res2.y == -20.0f / 8.0f);

        const Vec2 res3 = 30.0f / Vec2(10.0f, -5.0f);
        ASSERT(res3.x == 3.0f && res3.y == -6.0f);
    }
    {
        const Vec2 res1 = minVec(v, v2);
        ASSERT(res1.x == 2.0f && res1.y == -3.0f);

        const Vec2 res2 = maxVec(v, v2);
        ASSERT(res2.x == 5.0f && res2.y == 7.0f);

        const float res3 = minVec(v);
        ASSERT(res3 == -3.0f);

        const float res4 = maxVec(v);
        ASSERT(res4 == 2.0f);
    }

    {
        const Vec2 vec(3.0f, 4.0f);
        ASSERT(sqrLen(vec) == 25.0f);

        ASSERT(len(vec) - 5.0f < VecEpsilon);
    }

    {
        const float d = dot(v, v2);
        ASSERT(d == 10.0f - 21.0f);
    }

    {
        const Vec2 res = normalize(Vec2(3.0f, 4.0f));
        ASSERT(res.x == 3.0f / 5.0f && res.y == 4.0f / 5.0f);
    }
}

void testVec3()
{
    const Vec3 v(2.0f, -3.0f, 11.0f);
    const Vec3 v2(5.0f, 7.0f, 13.0f);
    {
        ASSERT(v.x == 2.0f && v.y == -3.0f && v.z == 11.0f);
    }

    {
        const Vec3 res1 = -v;
        ASSERT(res1.x == -2.0f && res1.y == 3.0f && res1.z == -11.0f);

        const Vec3 res2 = v - v2;
        ASSERT(res2.x == -3.0f && res2.y == -10.0f && res2.z == -2.0f);

        const Vec3 res3 = v - 10.0f;
        ASSERT(res3.x == -8.0f && res3.y == -13.0f && res3.z == 1.0f);
    }

    {
        const Vec3 res1 = v + v2;
        ASSERT(res1.x == 7.0f && res1.y == 4.0f && res1.z == 24.0f);

        const Vec3 res2 = v + 11.0f;
        ASSERT(res2.x == 13.0f && res2.y == 8.0f && res2.z == 22.0f);

        const Vec3 res3 = 13.0f + v;
        ASSERT(res3.x == 15.0f && res3.y == 10.0f && res3.z == 24.0f);
    }
    {
        const Vec3 res1 = v * v2;
        ASSERT(res1.x == 10.0f && res1.y == -21.0f && res1.z == 143.0f);

        const Vec3 res2 = v * 19.0f;
        ASSERT(res2.x == 38.0f && res2.y == -57.0f && res2.z == 209.0f);

        const Vec3 res3 = 17.0f * v;
        ASSERT(res3.x == 34.0f && res3.y == -51.0f && res3.z == 187.0f);
    }

    {
        const Vec3 vec1(9.0f, -20.0f, 56.0f);
        const Vec3 vec2(3.0f, 4.0f, 7.0f);

        const Vec3 res1 = vec1 / vec2;
        ASSERT(res1.x == 3.0f && res1.y == -5.0f && res1.z == 8.0f);

        const Vec3 res2 = vec1 / 8.0f;
        ASSERT(res2.x == 9.0f / 8.0f && res2.y == -20.0f / 8.0f && res2.z == 7.0f);

        const Vec3 res3 = 30.0f / Vec3(10.0f, -5.0f, 15.0f);
        ASSERT(res3.x == 3.0f && res3.y == -6.0f && res3.z == 2.0f);
    }
    {
        const Vec3 res1 = minVec(v, v2);
        ASSERT(res1.x == 2.0f && res1.y == -3.0f && res1.z == 11.0f);

        const Vec3 res2 = maxVec(v, v2);
        ASSERT(res2.x == 5.0f && res2.y == 7.0f && res2.z == 13.0f);

        const float res3 = minVec(v);
        ASSERT(res3 == -3.0f);

        const float res4 = maxVec(v);
        ASSERT(res4 == 11.0f);
    }

    {
        const Vec3 vec(1.0f, -2.0f, 2.0f);
        ASSERT(sqrLen(vec) == 9.0f);

        ASSERT(len(vec) - 3.0f < VecEpsilon);
    }

    {
        const float d = dot(v, v2);
        ASSERT(d == 10.0f - 21.0f + 143.0f);
    }

    {
        const Vec3 res = normalize(Vec3(1.0f, -2.0f, 2.0f));
        ASSERT(fabs(res.x - 1.0f / 3.0f) < VecEpsilon && fabs(res.y + 2.0f / 3.0f) < VecEpsilon && fabs(res.z - 2.0f / 3.0f) < VecEpsilon);
    }
}

void testVec4()
{
    const Vec4 v(2.0f, -3.0f, 11.0f, 27.0f);
    const Vec4 v2(5.0f, 7.0f, 13.0f, 23.0f);
    {
        ASSERT(v.x == 2.0f && v.y == -3.0f && v.z == 11.0f && v.w == 27.0f);
    }

    {
        const Vec4 res1 = -v;
        ASSERT(res1.x == -2.0f && res1.y == 3.0f && res1.z == -11.0f && res1.w == -27.0f);

        const Vec4 res2 = v - v2;
        ASSERT(res2.x == -3.0f && res2.y == -10.0f && res2.z == -2.0f && res2.w == 4.0f);

        const Vec4 res3 = v - 10.0f;
        ASSERT(res3.x == -8.0f && res3.y == -13.0f && res3.z == 1.0f && res3.w == 17.0f);
    }

    {
        const Vec4 res1 = v + v2;
        ASSERT(res1.x == 7.0f && res1.y == 4.0f && res1.z == 24.0f && res1.w == 50.0f);

        const Vec4 res2 = v + 11.0f;
        ASSERT(res2.x == 13.0f && res2.y == 8.0f && res2.z == 22.0f && res2.w == 38.0f);

        const Vec4 res3 = 13.0f + v;
        ASSERT(res3.x == 15.0f && res3.y == 10.0f && res3.z == 24.0f && res3.w == 40.0f);
    }
    {
        const Vec4 res1 = v * v2;
        ASSERT(res1.x == 10.0f && res1.y == -21.0f && res1.z == 143.0f && res1.w == 621.0f);

        const Vec4 res2 = v * 19.0f;
        ASSERT(res2.x == 38.0f && res2.y == -57.0f && res2.z == 209.0f && res2.w == 513.0f);

        const Vec4 res3 = 17.0f * v;
        ASSERT(res3.x == 34.0f && res3.y == -51.0f && res3.z == 187.0f && res3.w == 459.0f);
    }

    {
        const Vec4 vec1(9.0f, -20.0f, 56.0f, 72.0f);
        const Vec4 vec2(3.0f, 4.0f, 7.0f, 9.0f);

        const Vec4 res1 = vec1 / vec2;
        ASSERT(res1.x == 3.0f && res1.y == -5.0f && res1.z == 8.0f && res1.w == 8.0f);

        const Vec4 res2 = vec1 / 8.0f;
        ASSERT(res2.x == 9.0f / 8.0f && res2.y == -20.0f / 8.0f && res2.z == 7.0f && res2.w == 9.0f);

        const Vec4 res3 = 30.0f / Vec4(10.0f, -5.0f, 15.0f, -60.0f);
        ASSERT(res3.x == 3.0f && res3.y == -6.0f && res3.z == 2.0f && res3.w == -0.5f);
    }
    {
        const Vec4 res1 = minVec(v, v2);
        ASSERT(res1.x == 2.0f && res1.y == -3.0f && res1.z == 11.0f && res1.w == 23.0f);

        const Vec4 res2 = maxVec(v, v2);
        ASSERT(res2.x == 5.0f && res2.y == 7.0f && res2.z == 13.0f && res2.w == 27.0f);

        const float res3 = minVec(v);
        ASSERT(res3 == -3.0f);

        const float res4 = maxVec(v);
        ASSERT(res4 == 27.0f);
    }

    {
        const Vec4 vec(1.0f, -2.0f, 2.0f, -4.0f);
        ASSERT(sqrLen(vec) == 25.0f);

        ASSERT(len(vec) - 5.0 < VecEpsilon);
    }

    {
        const float d = dot(v, v2);
        ASSERT(d == 10.0f - 21.0f + 143.0f + 621.0f);
    }

    {
        const Vec4 res = normalize(Vec4(1.0f, -2.0f, 2.0f, -4.0f));
        ASSERT(
            fabs(res.x - 1.0f / 5.0f) < VecEpsilon &&
            fabs(res.y + 2.0f / 5.0f) < VecEpsilon &&
            fabs(res.z - 2.0f / 5.0f) < VecEpsilon &&
            fabs(res.w + 4.0f / 5.0f) < VecEpsilon);
    }
}

void testMathVector()
{
    testVec2();
    testVec3();
    testVec4();
}