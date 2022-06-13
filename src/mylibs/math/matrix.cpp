#include "matrix.h"
#include <math/quaternion.h>
#include <math/general_math.h>

#include <cmath>

#include <xmmintrin.h>
#include <immintrin.h>

#include <core/timer.h>

static Timer matrixTimer(Timer::ClockType::ClockCoarseId);

Matrix getMatrixFromRotation(const Vector3 &right, const Vector3 &up, const Vector3 &forward)
{
    Matrix result{ Uninit };

    result._00 = right.x;
    result._01 = up.x;
    result._02 = forward.x;

    result._10 = right.y;
    result._11 = up.y;
    result._12 = forward.y;

    result._20 = right.z;
    result._21 = up.z;
    result._22 = forward.z;

    result._03 = 0.0f;
    result._13 = 0.0f;
    result._23 = 0.0f;

    result._30 = 0.0f;
    result._31 = 0.0f;
    result._32 = 0.0f;
    result._33 = 1.0f;
    return result;
}

Matrix getMatrixFromQuaternionLH(const Quaternion &quat)
{
    Matrix result{ Uninit };
    result._00 = 1.0f - 2.0f * quat.v.y * quat.v.y - 2.0f * quat.v.z * quat.v.z;
    result._01 = 2.0f * quat.v.x * quat.v.y + 2.0f * quat.w * quat.v.z;
    result._02 = 2.0f * quat.v.x * quat.v.z - 2.0f * quat.w * quat.v.y;

    result._10 = 2.0f * quat.v.x * quat.v.y - 2.0f * quat.w * quat.v.z;
    result._11 = 1.0f - 2.0f * quat.v.x * quat.v.x - 2.0f * quat.v.z * quat.v.z;
    result._12 = 2.0f * quat.v.y * quat.v.z + 2.0f * quat.w * quat.v.x;

    result._20 = 2.0f * quat.v.x * quat.v.z + 2.0f * quat.w * quat.v.y;
    result._21 = 2.0f * quat.v.y * quat.v.z - 2.0f * quat.w * quat.v.x;
    result._22 = 1.0f - 2.0f * quat.v.x * quat.v.x - 2.0f * quat.v.y * quat.v.y;

    result._03 = 0.0f;
    result._13 = 0.0f;
    result._23 = 0.0f;

    result._30 = 0.0f;
    result._31 = 0.0f;
    result._32 = 0.0f;
    result._33 = 1.0f;
    return result;
}

Matrix getMatrixFromQuaternion(const Quaternion &quat)
{
    Matrix result{ Uninit };
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

    result._30 = 0.0f;
    result._31 = 0.0f;
    result._32 = 0.0f;
    result._33 = 1.0f;
    return result;
}


Matrix getMatrixFromScale(const Vector3 &scale)
{
    Matrix result;
    result._00 = scale.x;
    result._11 = scale.y;
    result._22 = scale.z;

    return result;
}

Matrix getMatrixFromTranslation(const Vector3 &pos)
{
    Matrix result;
    result._03 = pos.x;
    result._13 = pos.y;
    result._23 = pos.z;

    return result;

}

Matrix createOrthoMatrix(float width, float height, float nearPlane, float farPlane)
{
    Matrix result;

    ASSERT(ffabsf(width) >= 1.0f);
    ASSERT(ffabsf(height) >= 1.0f);
    ASSERT(ffabsf(farPlane - nearPlane) > 0.00001f);

    float fRange = 1.0f / (farPlane - nearPlane);

    result._00 = 2.0f / width;
    result._11 = 2.0f / height;
    result._22 = fRange;
    result._23 = -fRange * nearPlane;
    result._33 = 1.0f;
    return result;
}

Matrix createPerspectiveMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    Matrix result;
    ASSERT(ffabsf(fov) > 0.00001f);
    ASSERT(ffabsf(aspectRatio) > 0.001f);
    ASSERT(ffabsf(farPlane - nearPlane) > 0.00001f);
    ASSERT(ffabsf(nearPlane) > 0.0f);

    float yScale = 1.0f / ftanf(toRadians(fov * 0.5f));
    float xScale = yScale / aspectRatio;
    float fRange = farPlane / (farPlane - nearPlane);

    result._00 = xScale;
    result._11 = yScale;

    result._22 = -fRange;
    result._23 = -nearPlane * fRange;
    result._32 = -1.0f;
    result._33 = 0.0f;
    return result;
}

Matrix createMatrixFromLookAt(const Vec3 &pos, const Vec3 &target, const Vec3 &up)
{
    const Vec3 forward = -normalize(target - pos);
    const Vec3 right = normalize(cross(up, forward));
    const Vec3 realUp = normalize(cross(forward, right));

    Matrix result{ Uninit };
    result._00 = right.x;
    result._01 = right.y;
    result._02 = right.z;
    result._03 = -dot(pos, right);

    result._10 = realUp.x;
    result._11 = realUp.y;
    result._12 = realUp.z;
    result._13 = -dot(pos, realUp);

    result._20 = forward.x;
    result._21 = forward.y;
    result._22 = forward.z;
    result._23 = -dot(pos, forward);

    result._30 = 0.0;
    result._31 = 0.0;
    result._32 = 0.0;
    result._33 = 1.0;
    return result;
}


Matrix transpose(const Matrix &m)
{
    Matrix result{Uninit};
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

Matrix operator*(const Matrix &a, const Matrix &b)
{
    //matrixTimer.continueTimer();
    Matrix result{ Uninit };

#if (__AVX__ || __SSE__ || __SSE2__ || __SSE3__ || __SSE4_1__ || _M_AMD64 || _M_X64) && 1

    __m128 aRows[4];
    aRows[0] = _mm_load_ps(&a._00);
    aRows[1] = _mm_load_ps(&a._10);
    aRows[2] = _mm_load_ps(&a._20);
    aRows[3] = _mm_load_ps(&a._30);

    __m128 bR0 = _mm_load_ps(&b._00);
    __m128 bR1 = _mm_load_ps(&b._10);
    __m128 bR2 = _mm_load_ps(&b._20);
    __m128 bR3 = _mm_load_ps(&b._30);

    for(uint32_t i = 0; i < 4; ++i)
    {
        __m128 r0 = _mm_mul_ps(_mm_shuffle_ps(aRows[i], aRows[i], _MM_SHUFFLE(0, 0, 0, 0)), bR0);
        __m128 r1 = _mm_mul_ps(_mm_shuffle_ps(aRows[i], aRows[i], _MM_SHUFFLE(1, 1, 1, 1)), bR1);
        __m128 r2 = _mm_mul_ps(_mm_shuffle_ps(aRows[i], aRows[i], _MM_SHUFFLE(2, 2, 2, 2)), bR2);
        __m128 r3 = _mm_mul_ps(_mm_shuffle_ps(aRows[i], aRows[i], _MM_SHUFFLE(3, 3, 3, 3)), bR3);
        __m128 rRes = _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));
        _mm_store_ps(&result._00 + i * 4, rRes);
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
    //matrixTimer.pauseTimer();
    return result;
}

bool operator==(const Matrix &a, const Matrix &b)
{
    static constexpr float EPS_DIFF = 5.0e-2f;

    for(uint32_t i = 0; i < 16; ++i)
    {
        float f = fabsf(a[i] - b[i]);
        if(f > EPS_DIFF)
            return false;
    }
    return true;
}

Vec4 mul(const Matrix& m, const Vec4& v)
{
    Vec4 result {Uninit};
    result.x = v.x * m._00 + v.y * m._01 + v.z * m._02 + v.w * m._03;
    result.y = v.x * m._10 + v.y * m._11 + v.z * m._12 + v.w * m._13;
    result.z = v.x * m._20 + v.y * m._21 + v.z * m._22 + v.w * m._23;
    result.w = v.x * m._30 + v.y * m._31 + v.z * m._32 + v.w * m._33;
    return result;
}
Vec4 mul(const Vec4& v, const Matrix& m)
{
    Vec4 result{ Uninit };
    result.x = v.x * m._00 + v.y * m._10 + v.z * m._20 + v.w * m._30;
    result.y = v.x * m._01 + v.y * m._11 + v.z * m._21 + v.w * m._31;
    result.z = v.x * m._02 + v.y * m._12 + v.z * m._22 + v.w * m._32;
    result.w = v.x * m._03 + v.y * m._13 + v.z * m._23 + v.w * m._33;
    return result;
}

Matrix inverse(const Matrix& m)
{
    Matrix inv{Uninit};
    inv[0] = (
        (m[5]  * m[10] * m[15] - m[5]  * m[11] * m[14]) -
        (m[9]  * m[6]  * m[15] - m[9]  * m[7]  * m[14]) +
        (m[13] * m[6]  * m[11] - m[13] * m[7]  * m[10]));

    inv[1] = -(
        (m[1]  * m[10] * m[15] - m[1]  * m[11] * m[14]) -
        (m[9]  * m[2]  * m[15] - m[9]  * m[3]  * m[14]) +
        (m[13] * m[2]  * m[11] - m[13] * m[3]  * m[10]));

    inv[2] = (
        (m[1]  * m[6] * m[15] - m[1]  * m[7] * m[14]) -
        (m[5]  * m[2] * m[15] - m[5]  * m[3] * m[14]) +
        (m[13] * m[2] * m[7]  - m[13] * m[3] * m[6]));

    inv[3] = -(
        (m[1] * m[6] * m[11] - m[1] * m[7] * m[10]) -
        (m[5] * m[2] * m[11] - m[5] * m[3] * m[10]) +
        (m[9] * m[2] * m[7]  - m[9] * m[3] * m[6]));




    inv[4] = - (
        (m[4]  * m[10] * m[15] - m[4]  * m[11] * m[14]) -
        (m[8]  * m[6]  * m[15] - m[8]  * m[7]  * m[14]) +
        (m[12] * m[6]  * m[11] - m[12] * m[7]  * m[10]));

    inv[5] = (
        (m[0]  * m[10] * m[15] - m[0]  * m[11] * m[14]) -
        (m[8]  * m[2]  * m[15] - m[8]  * m[3]  * m[14]) +
        (m[12] * m[2]  * m[11] - m[12] * m[3]  * m[10]));

    inv[6] = -(
        (m[0] * m[6] * m[15] - m[0] * m[7] * m[14]) -
        (m[4] * m[2] * m[15] - m[4] * m[3] * m[14]) +
        (m[12] * m[2] * m[7] - m[12] * m[3] * m[6]));

    inv[7] = (
        (m[0] * m[6] * m[11] - m[0] * m[7] * m[10]) -
        (m[4] * m[2] * m[11] - m[4] * m[3] * m[10]) +
        (m[8] * m[2] * m[7]  - m[8] * m[3] * m[6]));




    inv[8] = (
        (m[4] * m[9] * m[15] - m[4] * m[11] * m[13]) -
        (m[8] * m[5] * m[15] - m[8] * m[7] * m[13]) +
        (m[12] * m[5] * m[11] - m[12] * m[7] * m[9]));

    inv[9] = -(
        (m[0] * m[9] * m[15] - m[0] * m[11] * m[13]) -
        (m[8] * m[1] * m[15] - m[8] * m[3] * m[13]) +
        (m[12] * m[1] * m[11] - m[12] * m[3] * m[9]));

    inv[10] = (
        (m[0] * m[5] * m[15] - m[0] * m[7] * m[13]) -
        (m[4] * m[1] * m[15] - m[4] * m[3] * m[13]) +
        (m[12] * m[1] * m[7] - m[12] * m[3] * m[5]));

    inv[11] = -(
        (m[0] * m[5] * m[11] - m[0] * m[7] * m[9]) -
        (m[4] * m[1] * m[11] - m[4] * m[3] * m[9]) +
        (m[8] * m[1] * m[7]  - m[8] * m[3] * m[5]));




    inv[12] = -(
        (m[4] * m[9] * m[14] - m[4] * m[10] * m[13]) -
        (m[8] * m[5] * m[14] - m[8] * m[6] * m[13]) +
        (m[12] * m[5] * m[10] - m[12] * m[6] * m[9]));

    inv[13] = (
        (m[0] * m[9] * m[14] - m[0] * m[10] * m[13]) -
        (m[8] * m[1] * m[14] - m[8] * m[2] * m[13]) +
        (m[12] * m[1] * m[10] - m[12] * m[2] * m[9]));

    inv[14] = -(
        (m[0] * m[5] * m[14] - m[0] * m[6] * m[13]) -
        (m[4] * m[1] * m[14] - m[4] * m[2] * m[13]) +
        (m[12] * m[1] * m[6] - m[12] * m[2] * m[5]));

    inv[15] = (
        (m[0] * m[5] * m[10] - m[0] * m[6] * m[9]) -
        (m[4] * m[1] * m[10] - m[4] * m[2] * m[9]) +
        (m[8] * m[1] * m[6]  - m[8] * m[2] * m[5]));

    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    ASSERT(det != 0.0f);
    if (det == 0)
        return Matrix();

    det = 1.0f / det;
    Matrix result;
    for (uint32_t i = 0; i < 16; i++)
        result[i] = inv[i] * det;

    return result;
}


bool isIdentity(const Matrix &m)
{
    static constexpr float EPS_DIFF = 1.0e-4f;

    for(uint32_t i = 0; i < 16; ++i)
    {
        float f = m[i];

        if(i == (i / 4) + (i / 4) * 4)
            f = f - 1.0f;

        if(fabsf(f) > EPS_DIFF)
            return false;
    }
    return true;
}

void printMatrix(const Matrix &m, const char name[])
{
    LOG("%s\n", name);
    LOG("%f, %f, %f, %f\n", m._00, m._01, m._02, m._03);
    LOG("%f, %f, %f, %f\n", m._10, m._11, m._12, m._13);
    LOG("%f, %f, %f, %f\n", m._20, m._21, m._22, m._23);
    LOG("%f, %f, %f, %f\n", m._30, m._31, m._32, m._33);
    LOG("\n");
}


double getMatrixTime()
{
    double v = matrixTimer.getDuration();
    matrixTimer.resetTimer();
    return v;

}
