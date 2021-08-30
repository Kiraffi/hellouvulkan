
#include "camera.h"
#include "math/matrix.h"
#include "math/general_math.h"


#include <math.h>

Matrix perspectiveProjection(const Camera &camera)
{;
	float fovY = toRadians(camera.fovY);
	float f = 1.0f / tanf(fovY / 2.0f);

	float farMinusNear = camera.zFar - camera.zNear;
	float s1 = -camera.zFar / farMinusNear;
	float s2 = -camera.zFar * camera.zNear / farMinusNear;

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
