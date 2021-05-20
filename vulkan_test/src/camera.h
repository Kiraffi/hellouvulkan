#pragma once

#include <math/vector3.h>
#include <math/matrix.h>

struct Camera
{
	Vec3 forwardDir = Vec3(0.0f, 0.0f, 1.0f);
	Vec3 upDir = Vec3(0.0f, 1.0f, 0.0f);
	Vec3 position = Vec3(0.0f, 0.0f, -5.0f);

	float aspectRatioWByH = 1.667f;
	float fovY = 100.0f;

	float pitch = 0.0f;
	float yaw = 0.0f;
	float roll = 0.0f;

	float zNear = 0.1f;
	float zFar = 2000.0f; // infinite?
};


Matrix perspectiveProjection(const Camera &camera);
