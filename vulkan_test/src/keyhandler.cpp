#include "keyhandler.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>

#include "camera.h"
#include <core/mytypes.h>

static bool keysDown[512] = {};

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(action == GLFW_PRESS)
	{
		if(key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose( window, 1 );
		else if(key >= 0 && key < 512)
			keysDown[key] = true;
	}
	else if(action == GLFW_RELEASE && key >= 0 && key < 512)
	{
		keysDown[key] = false;
	}
}


void checkKeypresses(float deltaTime, Camera &camera)
{
	//printf("deltatime: %f\n", deltaTime);
	float moveBooster = keysDown[GLFW_KEY_LEFT_SHIFT] ? 5.0f : 1.0f;
	float rotationBooster = keysDown[GLFW_KEY_LEFT_SHIFT] ? 2.0f : 1.0f;

	float moveSpeed = deltaTime * 2.0f * moveBooster;
	float rotationSpeed = deltaTime * 1.0f * rotationBooster;

 	if(keysDown[GLFW_KEY_I])
	{
		camera.pitch -= rotationSpeed;
	}
	if(keysDown[GLFW_KEY_K])
	{
		camera.pitch += rotationSpeed;
	}
	if(keysDown[GLFW_KEY_J])
	{
		camera.yaw -= rotationSpeed;
	}
	if(keysDown[GLFW_KEY_L])
	{
		camera.yaw += rotationSpeed;
	}

	camera.pitch = glm::clamp(camera.pitch, -0.5f * pii, 0.5f * pii);
	camera.yaw = std::fmod(camera.yaw, 2.0f * pii);


	glm::quat cameraRotation = glm::angleAxis(camera.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
	cameraRotation = cameraRotation * glm::angleAxis(camera.pitch, glm::vec3(1.0f, 0.0f, 0.0f));
	camera.forwardDir = cameraRotation * glm::vec3(0.0, 0.0, 1.0f);
	camera.upDir =  cameraRotation * glm::vec3(0.0, 1.0, 0.0f);

	glm::vec3 cameraRight = cameraRotation * glm::vec3(1.0f, 0.0, 0.0f);

	if(keysDown[GLFW_KEY_W])
	{
		camera.position += camera.forwardDir * moveSpeed;
	}
	if(keysDown[GLFW_KEY_S])
	{
		camera.position -= camera.forwardDir * moveSpeed;
	}
	if(keysDown[GLFW_KEY_A])
	{
		camera.position -= cameraRight * moveSpeed;
	}
	if(keysDown[GLFW_KEY_D])
	{
		camera.position += cameraRight * moveSpeed;

	}
	if(keysDown[GLFW_KEY_Q])
	{
		camera.position += camera.upDir * moveSpeed;
	}
	if(keysDown[GLFW_KEY_E])
	{
		camera.position -= camera.upDir * moveSpeed;
	}
}

