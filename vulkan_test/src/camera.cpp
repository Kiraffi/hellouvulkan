
#include "camera.h"

#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>

glm::mat4 perspectiveProjection(const Camera &camera)
{;
	float fovY = glm::radians(camera.fovY);
	float f = 1.0f / tanf(fovY / 2.0f);
	return glm::mat4(
		f / camera.aspectRatioWByH, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, camera.zNear, 0.0f);
}
