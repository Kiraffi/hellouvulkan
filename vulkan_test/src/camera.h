#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct Camera
{
	glm::vec3 forwardDir = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 upDir = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 position = glm::vec3(0.0f, 0.0f, -5.0f);

	float aspectRatioWByH = 1.667f;
	float fovY = 100.0f;

	float pitch = 0.0f;
	float yaw = 0.0f;
	float roll = 0.0f;

	float zNear = 0.1f;
	float zFar = 2000.0f; // infinite?
};


glm::mat4 perspectiveProjection(const Camera &camera);
