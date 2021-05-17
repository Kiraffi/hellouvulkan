#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct Transform
{
	glm::vec3 pos;
	glm::vec3 scale;
	glm::quat rot;
};

glm::mat4 getModelMatrix(const Transform &trans)
{
	glm::mat4 posMat = glm::translate(glm::mat4(1.0f), trans.pos);

	glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f),trans.scale);
	glm::mat4 rotMat = glm::mat4_cast(trans.rot);

	return posMat * rotMat * scaleMat;
}
