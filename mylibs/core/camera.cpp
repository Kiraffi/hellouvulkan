
#include "camera.h"
#include "math/matrix.h"
#include "math/general_math.h"


#include <math.h>

Matrix perspectiveProjection(const Camera &camera)
{;
	float fovY = toRadians(camera.fovY * 0.5f);
	float f = 1.0f / tanf(fovY);

	float s1 = camera.zFar / (camera.zFar - camera.zNear);
	float s2 = -camera.zNear * s1;

	return Matrix(
		f / camera.aspectRatioWByH, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, s1, 1.0f,
		0.0f, 0.0f, s2, 0.0f);
}



Matrix perspectiveProjectionInf(const Camera& camera)
{
	;
	float fovY = toRadians(camera.fovY);
	float f = 1.0f / tanf(fovY / 2.0f);

	float farMinusNear = camera.zFar - camera.zNear;

	return Matrix(
		f / camera.aspectRatioWByH, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, camera.zNear, 0.0f);
}


Matrix getCameraMatrix2(const Camera& camera)
{
	Vector3 right = cross(camera.upDir, camera.forwardDir);
	
	Matrix result;
	result._00 = camera.rightDir.x;
	result._01 = camera.rightDir.y;
	result._02 = camera.rightDir.z;

	result._10 = camera.upDir.x;
	result._11 = camera.upDir.y;
	result._12 = camera.upDir.z;

	result._20 = camera.forwardDir.x;
	result._21 = camera.forwardDir.y;
	result._22 = camera.forwardDir.z;

	Matrix m;
	m._03 = camera.position.x;
	m._13 = camera.position.y;
	m._23 = camera.position.z;

	return result * m;
}

Matrix getCameraMatrix(const Camera& camera)
{
	Vector3 right = cross(camera.upDir, camera.forwardDir);

	Matrix result;
	result._00 = camera.rightDir.x;
	result._01 = camera.upDir.x;
	result._02 = camera.forwardDir.x;

	result._10 = camera.rightDir.y;
	result._11 = camera.upDir.y;
	result._12 = camera.forwardDir.y;

	result._20 = camera.rightDir.z;
	result._21 = camera.upDir.z;
	result._22 = camera.forwardDir.z;


	result._30 = -dot(camera.position, camera.rightDir);
	result._31 = -dot(camera.position, camera.upDir);
	result._32 = -dot(camera.position, camera.forwardDir);

	return result;
}