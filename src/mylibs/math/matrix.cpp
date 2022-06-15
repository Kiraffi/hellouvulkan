#include "matrix.h"

#include <core/timer.h>
#include <math/general_math.h>
#include <math/quaternion.h>
#include <math/vector3_inline_functions.h>

#include <math.h>


Matrix::Matrix(const Mat3x4 &mat)
    : _00(mat._00), _01(mat._01), _02(mat._02), _03(mat._03)
    , _10(mat._10), _11(mat._11), _12(mat._12), _13(mat._13)
    , _20(mat._20), _21(mat._21), _22(mat._22), _23(mat._23)
    , _30(0.0f), _31(0.0f), _32(0.0f), _33(1.0f)
{}

Mat3x4::Mat3x4(const Matrix &mat) 
    : _00(mat._00), _01(mat._01), _02(mat._02), _03(mat._03)
    , _10(mat._10), _11(mat._11), _12(mat._12), _13(mat._13)
    , _20(mat._20), _21(mat._21), _22(mat._22), _23(mat._23)
{
    ASSERT(mat._30 == 0.0f && mat._31 == 0.0f && mat._32 == 0.0f && mat._33 == 1.0f);
}


Mat3x4 getMatrixFromQuaternionLH(const Quaternion &quat)
{
    Mat3x4 result{ Uninit };
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
    Matrix result{ Uninit };
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

void printMatrix(const Matrix &m, const char *name)
{
    LOG("%s\n", name);
    LOG("%f, %f, %f, %f\n", m._00, m._01, m._02, m._03);
    LOG("%f, %f, %f, %f\n", m._10, m._11, m._12, m._13);
    LOG("%f, %f, %f, %f\n", m._20, m._21, m._22, m._23);
    LOG("%f, %f, %f, %f\n", m._30, m._31, m._32, m._33);
    LOG("\n");
}

void printMatrix(const Mat3x4 &m, const char *name)
{
    LOG("%s\n", name);
    LOG("%f, %f, %f, %f\n", m._00, m._01, m._02, m._03);
    LOG("%f, %f, %f, %f\n", m._10, m._11, m._12, m._13);
    LOG("%f, %f, %f, %f\n", m._20, m._21, m._22, m._23);
    LOG("\n");
}
