#pragma once

#include <core/mytypes.h>
#include <core/uninittype.h>
#include <math/vector3.h>

// The matrices are used mostly in row order where
// row 0 = x vec, transpose.x
// row 1 = y vec, transpose.y
// row 2 = z vec, transpose.z
// row 3 = projection stuff.
// It makes easy to convert to 4x3 or from 4x3 to 4x4,
// since no need to transpose and shaders use rowmatrix,
// so its direct copy.
struct alignas(16) Mat3x4;

struct alignas(16) Matrix
{
    Matrix() :
        _00(1.0f), _01(0.0f), _02(0.0f), _03(0.0f),
        _10(0.0f), _11(1.0f), _12(0.0f), _13(0.0f),
        _20(0.0f), _21(0.0f), _22(1.0f), _23(0.0f),
        _30(0.0f), _31(0.0f), _32(0.0f), _33(1.0f) {}
    Matrix(UninitType) {}

    Matrix(
        float f00, float f01, float f02, float f03,
        float f10, float f11, float f12, float f13,
        float f20, float f21, float f22, float f23,
        float f30, float f31, float f32, float f33)
        : _00(f00), _01(f01), _02(f02), _03(f03)
        , _10(f10), _11(f11), _12(f12), _13(f13)
        , _20(f20), _21(f21), _22(f22), _23(f23)
        , _30(f30), _31(f31), _32(f32), _33(f33)
    {}
    Matrix(const Mat3x4 &mat);

    float &operator[](uint32_t index) { return (&_00)[index]; }
    float operator[](uint32_t index) const { return (&_00)[index]; }

    float _00;
    float _01;
    float _02;
    float _03;

    float _10;
    float _11;
    float _12;
    float _13;

    float _20;
    float _21;
    float _22;
    float _23;

    float _30;
    float _31;
    float _32;
    float _33;

};

struct alignas(16) Mat3x4
{
    Mat3x4() :
        _00(1.0f), _01(0.0f), _02(0.0f), _03(0.0f),
        _10(0.0f), _11(1.0f), _12(0.0f), _13(0.0f),
        _20(0.0f), _21(0.0f), _22(1.0f), _23(0.0f) {}
    Mat3x4(UninitType) {}

    Mat3x4(
        float f00, float f01, float f02, float f03,
        float f10, float f11, float f12, float f13,
        float f20, float f21, float f22, float f23)
        : _00(f00), _01(f01), _02(f02), _03(f03)
        , _10(f10), _11(f11), _12(f12), _13(f13)
        , _20(f20), _21(f21), _22(f22), _23(f23)
    {}

    Mat3x4(const Matrix &mat);

    float &operator[](uint32_t index) { return (&_00)[index]; }
    float operator[](uint32_t index) const { return (&_00)[index]; }

    float _00;
    float _01;
    float _02;
    float _03;

    float _10;
    float _11;
    float _12;
    float _13;

    float _20;
    float _21;
    float _22;
    float _23;
};

/*
Mat3x4 getMatrixFromQuaternion(const Quaternion &quat);
Mat3x4 getMatrixFromQuaternionLH(const Quaternion &quat);
Mat3x4 getMatrixFromScale(const Vector3 &scale);
Mat3x4 getMatrixFromTranslation(const Vector3 &pos);
*/

Matrix createOrthoMatrix(float width, float height, float nearPlane, float farPlane);
Matrix createPerspectiveMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);

Matrix createMatrixFromLookAt(const Vector3 &pos, const Vector3 &target, const Vector3 &up);

/*
Matrix transpose(const Matrix &m);
Matrix operator*(const Matrix &a, const Matrix &b);
*/

bool operator==(const Matrix &a, const Matrix &b);
Matrix inverse(const Matrix &m);

bool isIdentity(const Matrix &m);

/*
Vec4 mul(const Matrix& m, const Vec4& v);
Vec4 mul(const Vec4& v, const Matrix& m);

Vec4 mul(const Mat3x4 &m, const Vec4 &v);
Vec4 mul(const Vec4 &v, const Mat3x4 &m);

Matrix operator*(const Matrix &a, const Mat3x4 &b);
Matrix operator*(const Mat3x4 &a, const Matrix &b);
Mat3x4 operator*(const Mat3x4 &a, const Mat3x4 &b);
*/

void printMatrix(const Matrix &m, const char *name);
void printMatrix(const Mat3x4 &m, const char *name);


using mat4 = Matrix;

