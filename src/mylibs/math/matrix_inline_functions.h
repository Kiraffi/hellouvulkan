#pragma once

#include "matrix.h"

#include <core/assert.h>
#include <core/general.h>

#include <math/quaternion.h>

#include <xmmintrin.h>
#include <immintrin.h>

static FORCE_INLINE Mat3x4 getMatrixFromQuaternion(const Quaternion &quat)
{
    Mat3x4 result{ Uninit };
    result._00 = 1.0f - 2.0f * quat.v.y * quat.v.y - 2.0f * quat.v.z * quat.v.z;
    result._01 = 2.0f * quat.v.x * quat.v.y - 2.0f * quat.w * quat.v.z;
    result._02 = 2.0f * quat.v.x * quat.v.z + 2.0f * quat.w * quat.v.y;

    result._10 = 2.0f * quat.v.x * quat.v.y + 2.0f * quat.w * quat.v.z;
    result._11 = 1.0f - 2.0f * quat.v.x * quat.v.x - 2.0f * quat.v.z * quat.v.z;
    result._12 = 2.0f * quat.v.y * quat.v.z - 2.0f * quat.w * quat.v.x;

    result._20 = 2.0f * quat.v.x * quat.v.z - 2.0f * quat.w * quat.v.y;
    result._21 = 2.0f * quat.v.y * quat.v.z + 2.0f * quat.w * quat.v.x;
    result._22 = 1.0f - 2.0f * quat.v.x * quat.v.x - 2.0f * quat.v.y * quat.v.y;

    result._03 = 0.0f;
    result._13 = 0.0f;
    result._23 = 0.0f;

    return result;
}


static FORCE_INLINE Mat3x4 getMatrixFromScale(const Vector3 &scale)
{
    Mat3x4 result;
    result._00 = scale.x;
    result._11 = scale.y;
    result._22 = scale.z;

    return result;
}

static FORCE_INLINE Mat3x4 getMatrixFromTranslation(const Vector3 &pos)
{
    Mat3x4 result;
    result._03 = pos.x;
    result._13 = pos.y;
    result._23 = pos.z;

    return result;

}

static FORCE_INLINE Matrix transpose(const Matrix &m)
{
    Matrix result{ Uninit };
    result._00 = m._00;
    result._01 = m._10;
    result._02 = m._20;
    result._03 = m._30;

    result._10 = m._01;
    result._11 = m._11;
    result._12 = m._21;
    result._13 = m._31;

    result._20 = m._02;
    result._21 = m._12;
    result._22 = m._22;
    result._23 = m._32;

    result._30 = m._03;
    result._31 = m._13;
    result._32 = m._23;
    result._33 = m._33;

    return result;
}


static FORCE_INLINE Vec4 mul(const Matrix &m, const Vec4 &v)
{
    Vec4 result{ Uninit };
    result.x = v.x * m._00 + v.y * m._01 + v.z * m._02 + v.w * m._03;
    result.y = v.x * m._10 + v.y * m._11 + v.z * m._12 + v.w * m._13;
    result.z = v.x * m._20 + v.y * m._21 + v.z * m._22 + v.w * m._23;
    result.w = v.x * m._30 + v.y * m._31 + v.z * m._32 + v.w * m._33;
    return result;
}
static FORCE_INLINE Vec4 mul(const Vec4 &v, const Matrix &m)
{
    Vec4 result{ Uninit };
    result.x = v.x * m._00 + v.y * m._10 + v.z * m._20 + v.w * m._30;
    result.y = v.x * m._01 + v.y * m._11 + v.z * m._21 + v.w * m._31;
    result.z = v.x * m._02 + v.y * m._12 + v.z * m._22 + v.w * m._32;
    result.w = v.x * m._03 + v.y * m._13 + v.z * m._23 + v.w * m._33;
    return result;
}

static FORCE_INLINE Vec4 mul(const Mat3x4 &m, const Vec4 &v)
{
    Vec4 result{ Uninit };
    result.x = v.x * m._00 + v.y * m._01 + v.z * m._02 + v.w * m._03;
    result.y = v.x * m._10 + v.y * m._11 + v.z * m._12 + v.w * m._13;
    result.z = v.x * m._20 + v.y * m._21 + v.z * m._22 + v.w * m._23;
    result.w = v.w;
    return result;
}
static FORCE_INLINE Vec4 mul(const Vec4 &v, const Mat3x4 &m)
{
    Vec4 result{ Uninit };
    result.x = v.x * m._00 + v.y * m._10 + v.z * m._20;
    result.y = v.x * m._01 + v.y * m._11 + v.z * m._21;
    result.z = v.x * m._02 + v.y * m._12 + v.z * m._22;
    result.w = v.x * m._03 + v.y * m._13 + v.z * m._23 + v.w * 1.0f;
    return result;
}



static FORCE_INLINE Matrix operator*(const Matrix &a, const Matrix &b)
{
    Matrix result{ Uninit };

#if (__AVX__ || __SSE__ || __SSE2__ || __SSE3__ || __SSE4_1__ || _M_AMD64 || _M_X64) && 1

    const __m128 aRows0 = _mm_load_ps(&a._00);
    const __m128 aRows1 = _mm_load_ps(&a._10);
    const __m128 aRows2 = _mm_load_ps(&a._20);
    const __m128 aRows3 = _mm_load_ps(&a._30);

    const __m128 bR0 = _mm_load_ps(&b._00);
    const __m128 bR1 = _mm_load_ps(&b._10);
    const __m128 bR2 = _mm_load_ps(&b._20);
    const __m128 bR3 = _mm_load_ps(&b._30);
    {
        const __m128 r00 = _mm_mul_ps(_mm_shuffle_ps(aRows0, aRows0, _MM_SHUFFLE(0, 0, 0, 0)), bR0);
        const __m128 r01 = _mm_mul_ps(_mm_shuffle_ps(aRows0, aRows0, _MM_SHUFFLE(1, 1, 1, 1)), bR1);
        const __m128 r02 = _mm_mul_ps(_mm_shuffle_ps(aRows0, aRows0, _MM_SHUFFLE(2, 2, 2, 2)), bR2);
        const __m128 r03 = _mm_mul_ps(_mm_shuffle_ps(aRows0, aRows0, _MM_SHUFFLE(3, 3, 3, 3)), bR3);
        const __m128 r0Res = _mm_add_ps(_mm_add_ps(r00, r01), _mm_add_ps(r02, r03));
        _mm_store_ps(&result._00, r0Res);
    }
    {
        const __m128 r10 = _mm_mul_ps(_mm_shuffle_ps(aRows1, aRows1, _MM_SHUFFLE(0, 0, 0, 0)), bR0);
        const __m128 r11 = _mm_mul_ps(_mm_shuffle_ps(aRows1, aRows1, _MM_SHUFFLE(1, 1, 1, 1)), bR1);
        const __m128 r12 = _mm_mul_ps(_mm_shuffle_ps(aRows1, aRows1, _MM_SHUFFLE(2, 2, 2, 2)), bR2);
        const __m128 r13 = _mm_mul_ps(_mm_shuffle_ps(aRows1, aRows1, _MM_SHUFFLE(3, 3, 3, 3)), bR3);
        const __m128 r1Res = _mm_add_ps(_mm_add_ps(r10, r11), _mm_add_ps(r12, r13));
        _mm_store_ps(&result._10, r1Res);
    }
    {
        const __m128 r20 = _mm_mul_ps(_mm_shuffle_ps(aRows2, aRows2, _MM_SHUFFLE(0, 0, 0, 0)), bR0);
        const __m128 r21 = _mm_mul_ps(_mm_shuffle_ps(aRows2, aRows2, _MM_SHUFFLE(1, 1, 1, 1)), bR1);
        const __m128 r22 = _mm_mul_ps(_mm_shuffle_ps(aRows2, aRows2, _MM_SHUFFLE(2, 2, 2, 2)), bR2);
        const __m128 r23 = _mm_mul_ps(_mm_shuffle_ps(aRows2, aRows2, _MM_SHUFFLE(3, 3, 3, 3)), bR3);
        const __m128 r2Res = _mm_add_ps(_mm_add_ps(r20, r21), _mm_add_ps(r22, r23));
        _mm_store_ps(&result._20, r2Res);
    }
    {
        const __m128 r30 = _mm_mul_ps(_mm_shuffle_ps(aRows3, aRows3, _MM_SHUFFLE(0, 0, 0, 0)), bR0);
        const __m128 r31 = _mm_mul_ps(_mm_shuffle_ps(aRows3, aRows3, _MM_SHUFFLE(1, 1, 1, 1)), bR1);
        const __m128 r32 = _mm_mul_ps(_mm_shuffle_ps(aRows3, aRows3, _MM_SHUFFLE(2, 2, 2, 2)), bR2);
        const __m128 r33 = _mm_mul_ps(_mm_shuffle_ps(aRows3, aRows3, _MM_SHUFFLE(3, 3, 3, 3)), bR3);
        const __m128 r3Res = _mm_add_ps(_mm_add_ps(r30, r31), _mm_add_ps(r32, r33));
        _mm_store_ps(&result._30, r3Res);
    }
#else

#define MATRIX_ADD_ROW_MULT(row, col) (\
        a._##row##0 * b._0##col + \
        a._##row##1 * b._1##col + \
        a._##row##2 * b._2##col + \
        a._##row##3 * b._3##col)
#define MATRIX_SET(row, col) (result._##row##col)  = MATRIX_ADD_ROW_MULT(row, col)

    MATRIX_SET(0, 0);
    MATRIX_SET(0, 1);
    MATRIX_SET(0, 2);
    MATRIX_SET(0, 3);

    MATRIX_SET(1, 0);
    MATRIX_SET(1, 1);
    MATRIX_SET(1, 2);
    MATRIX_SET(1, 3);

    MATRIX_SET(2, 0);
    MATRIX_SET(2, 1);
    MATRIX_SET(2, 2);
    MATRIX_SET(2, 3);

    MATRIX_SET(3, 0);
    MATRIX_SET(3, 1);
    MATRIX_SET(3, 2);
    MATRIX_SET(3, 3);

#undef MATRIX_ADD_ROW_MULT
#undef MATRIX_SET

#endif
    return result;
}


static FORCE_INLINE Mat3x4 operator*(const Mat3x4 &a, const Mat3x4 &b)
{
    Mat3x4 result{ Uninit };

#if (__AVX__ || __SSE__ || __SSE2__ || __SSE3__ || __SSE4_1__ || _M_AMD64 || _M_X64) && 1

    const __m128 aRows0 = _mm_load_ps(&a._00);
    const __m128 aRows1 = _mm_load_ps(&a._10);
    const __m128 aRows2 = _mm_load_ps(&a._20);

    const __m128 bR0 = _mm_load_ps(&b._00);
    const __m128 bR1 = _mm_load_ps(&b._10);
    const __m128 bR2 = _mm_load_ps(&b._20);
    const __m128 bR3 = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
    {
        const __m128 r00 = _mm_mul_ps(_mm_shuffle_ps(aRows0, aRows0, _MM_SHUFFLE(0, 0, 0, 0)), bR0);
        const __m128 r01 = _mm_mul_ps(_mm_shuffle_ps(aRows0, aRows0, _MM_SHUFFLE(1, 1, 1, 1)), bR1);
        const __m128 r02 = _mm_mul_ps(_mm_shuffle_ps(aRows0, aRows0, _MM_SHUFFLE(2, 2, 2, 2)), bR2);
        const __m128 r03 = _mm_mul_ps(_mm_shuffle_ps(aRows0, aRows0, _MM_SHUFFLE(3, 3, 3, 3)), bR3);
        const __m128 r0Res = _mm_add_ps(_mm_add_ps(r00, r01), _mm_add_ps(r02, r03));
        _mm_store_ps(&result._00, r0Res);
    }
    {
        const __m128 r10 = _mm_mul_ps(_mm_shuffle_ps(aRows1, aRows1, _MM_SHUFFLE(0, 0, 0, 0)), bR0);
        const __m128 r11 = _mm_mul_ps(_mm_shuffle_ps(aRows1, aRows1, _MM_SHUFFLE(1, 1, 1, 1)), bR1);
        const __m128 r12 = _mm_mul_ps(_mm_shuffle_ps(aRows1, aRows1, _MM_SHUFFLE(2, 2, 2, 2)), bR2);
        const __m128 r13 = _mm_mul_ps(_mm_shuffle_ps(aRows1, aRows1, _MM_SHUFFLE(3, 3, 3, 3)), bR3);
        const __m128 r1Res = _mm_add_ps(_mm_add_ps(r10, r11), _mm_add_ps(r12, r13));
        _mm_store_ps(&result._10, r1Res);
    }
    {
        const __m128 r20 = _mm_mul_ps(_mm_shuffle_ps(aRows2, aRows2, _MM_SHUFFLE(0, 0, 0, 0)), bR0);
        const __m128 r21 = _mm_mul_ps(_mm_shuffle_ps(aRows2, aRows2, _MM_SHUFFLE(1, 1, 1, 1)), bR1);
        const __m128 r22 = _mm_mul_ps(_mm_shuffle_ps(aRows2, aRows2, _MM_SHUFFLE(2, 2, 2, 2)), bR2);
        const __m128 r23 = _mm_mul_ps(_mm_shuffle_ps(aRows2, aRows2, _MM_SHUFFLE(3, 3, 3, 3)), bR3);
        const __m128 r2Res = _mm_add_ps(_mm_add_ps(r20, r21), _mm_add_ps(r22, r23));
        _mm_store_ps(&result._20, r2Res);
    }

#else

#define MATRIX_ADD_ROW_MULT0(row, col) (\
        a._##row##0 * b._0##col + \
        a._##row##1 * b._1##col + \
        a._##row##2 * b._2##col)
#define MATRIX_ADD_ROW_MULT1(row, col) (\
        a._##row##0 * b._0##col + \
        a._##row##1 * b._1##col + \
        a._##row##2 * b._2##col + \
        a._##row##3)

#define MATRIX_SET0(row, col) (result._##row##col)  = MATRIX_ADD_ROW_MULT0(row, col)
#define MATRIX_SET1(row, col) (result._##row##col)  = MATRIX_ADD_ROW_MULT1(row, col)

    MATRIX_SET0(0, 0);
    MATRIX_SET0(0, 1);
    MATRIX_SET0(0, 2);
    MATRIX_SET1(0, 3);

    MATRIX_SET0(1, 0);
    MATRIX_SET0(1, 1);
    MATRIX_SET0(1, 2);
    MATRIX_SET1(1, 3);

    MATRIX_SET0(2, 0);
    MATRIX_SET0(2, 1);
    MATRIX_SET0(2, 2);
    MATRIX_SET1(2, 3);

#undef MATRIX_ADD_ROW_MULT0
#undef MATRIX_ADD_ROW_MULT1
#undef MATRIX_SET0
#undef MATRIX_SET1

#endif
    return result;
}

