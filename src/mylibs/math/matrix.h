#pragma once

#include "core/mytypes.h"
#include "math/vector3.h"

struct Quaternion;

struct Matrix
{
    Matrix() {}
    Matrix(float f) : _00(f), _11(f), _22(f), _33(f) {}
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
    float _00 = 1.0f;
    float _01 = 0.0f;
    float _02 = 0.0f;
    float _03 = 0.0f;

    float _10 = 0.0f;
    float _11 = 1.0f;
    float _12 = 0.0f;
    float _13 = 0.0f;

    float _20 = 0.0f;
    float _21 = 0.0f;
    float _22 = 1.0f;
    float _23 = 0.0f;

    float _30 = 0.0f;
    float _31 = 0.0f;
    float _32 = 0.0f;
    float _33 = 1.0f;

};

Matrix getMatrixFromRotation(const Vector3 &r, const Vector3 &u, const Vector3 &f);
Matrix getMatrixFromQuaternion(const Quaternion &quat);
Matrix getMatrixFromScale(const Vector3 &scale);
Matrix getMatrixFromTranslation(const Vector3 &pos);
Matrix createOrthoMatrix(float width, float height, float nearPlane, float farPlane);
Matrix createPerspectiveMatrixRH(float fov, float aspectRatio, float nearPlane, float farPlane);

Matrix createMatrixFromLookAt(const Vector3 &pos, const Vector3 &target, const Vector3 &up);

Matrix transpose(const Matrix &m);
Matrix operator*(const Matrix &a, const Matrix &b);

void printMatrix(const Matrix &m, const char name[]);

Vec4 mul(const Matrix& m, const Vec4& v);
Vec4 mul(const Vec4& v, const Matrix& m);

using mat4 = Matrix;