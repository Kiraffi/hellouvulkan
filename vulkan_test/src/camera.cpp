
#include "camera.h"
#include <math/matrix.h>
#include <math/general_math.h>


#include <cmath>

Matrix perspectiveProjection(const Camera &camera)
{;
	float fovY = toRadians(camera.fovY);
	float f = 1.0f / tanf(fovY / 2.0f);
	return Matrix(
		f / camera.aspectRatioWByH, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, camera.zNear, 0.0f);
}
