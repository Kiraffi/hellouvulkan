#include "matrix.h"
#include "math/quaternion.h"
#include <stdio.h>

#define MATRIX_ADD_ROW_MULT(row, col) (a._##row##0 * b._0##col + a._##row##1 * b._1##col + a._##row##2 * b._2##col + a._##row##3 * b._3##col)
#define MATRIX_SET(row, col) (result._##row##col)  = MATRIX_ADD_ROW_MULT(row, col)

Matrix getMatrixFromRotation(const Vector3 &right, const Vector3 &up, const Vector3 &forward)
{
	Matrix result;

	result._00 = right.x;
	result._01 = right.y;
	result._02 = right.z;

	result._10 = up.x;
	result._11 = up.y;
	result._12 = up.z;

	result._20 = forward.x;
	result._21 = forward.y;
	result._22 = forward.z;

	return result;
}

Matrix getMatrixFromQuaternion(const Quaternion &quat)
{
	Matrix result;
	result._00 = 1.0f - 2.0f * quat.v.y * quat.v.y - 2.0f * quat.v.z * quat.v.z;
	result._01 = 2.0f * quat.v.x * quat.v.y + 2.0f * quat.w * quat.v.z;
	result._02 = 2.0f * quat.v.x * quat.v.z - 2.0f * quat.w * quat.v.y;

	result._10 = 2.0f * quat.v.x * quat.v.y - 2.0f * quat.w * quat.v.z;
	result._11 = 1.0f - 2.0f * quat.v.x * quat.v.x - 2.0f * quat.v.z * quat.v.z;
	result._12 = 2.0f * quat.v.y * quat.v.z + 2.0f * quat.w * quat.v.x;

	result._20 = 2.0f * quat.v.x * quat.v.z + 2.0f * quat.w * quat.v.y;
	result._21 = 2.0f * quat.v.y * quat.v.z - 2.0f * quat.w * quat.v.x;
	result._22 = 1.0f - 2.0f * quat.v.x * quat.v.x - 2.0f * quat.v.y * quat.v.y;

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

	ASSERT(fabs(width) >= 1.0f);
	ASSERT(fabs(height) >= 1.0f);
	ASSERT(fabs(farPlane - nearPlane) > 0.00001f);

	float fRange = 1.0f / (farPlane - nearPlane);

	result._00 = 2.0f / width;
	result._11 = 2.0f / height;
	result._22 = fRange;
	result._32 = -fRange * nearPlane;
	//result._22 = 0.0f; // fRange
	//result._32 = 0.0f; // -fRange * NearZ;
	result._33 = 1.0f;
	return result;
}

Matrix createPerspectiveMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	Matrix result;
	ASSERT(fabs(fov) > 0.00001f);
	ASSERT(fabs(aspectRatio) > 0.001f);
	ASSERT(fabs(farPlane - nearPlane) > 0.00001f);

	float yScale = 1.0f / tanf(fov / 180.0f * PI);
	float xScale = yScale / aspectRatio;
	float fRange1 = 1.0f / (farPlane - nearPlane);
	float fRange2 = farPlane * fRange1;

	result._00 = xScale;
	result._11 = yScale;
//opengl
	result._22 = -fRange1 * (farPlane + nearPlane);
	result._23 = -2.0f * fRange2 * nearPlane;
/* dx
	result._22 = fRange2;
	result._23 = -fRange2 * nearPlane;
*/
	result._32 = -1.0f;
	result._33 = 0.0f;
	return result;
}

Matrix createMatrixFromLookAt(const Vector3 &pos, const Vector3 &target, const Vector3 &up)
{
	Vector3 forward = normalize(target - pos);
	Vector3 right = cross(up, forward);
	Vector3 realUp = cross(right, forward);

	Matrix result;
	result._00 = right.x;
	result._01 = right.y;
	result._02 = right.z;

	result._10 = realUp.x;
	result._11 = realUp.y;
	result._12 = realUp.z;

	result._20 = forward.x;
	result._21 = forward.y;
	result._22 = forward.z;

	return result;
}

Matrix transpose(const Matrix &m)
{
	Matrix result;
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
	Matrix result;

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

	return result;
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

