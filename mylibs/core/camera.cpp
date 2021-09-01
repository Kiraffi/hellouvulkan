
#include "camera.h"
#include "math/matrix.h"
#include "math/general_math.h"
#include "math/quaternion.h"

#include "render/font_render.h"

#include <math.h>
#include <stdio.h>

Matrix Camera::perspectiveProjectionRH()
{
	float f = 1.0f / tanf(toRadians(fovY * 0.5f));

	float s1 = -zFar / (zFar - zNear);
	float s2 = zNear * s1;

	return Matrix(
		f / aspectRatioWByH, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, s1, -1.0f,
		0.0f, 0.0f, s2, 0.0f);
}


Matrix Camera::getCameraMatrix()
{
	
	Vector3 rightDir;
	Vector3 upDir;
	Vector3 forwardDir;
	getCameraDirections(rightDir, upDir, forwardDir);

	Matrix result;
	result._00 = rightDir.x;
	result._01 = upDir.x;
	result._02 = forwardDir.x;

	result._10 = rightDir.y;
	result._11 = upDir.y;
	result._12 = forwardDir.y;

	result._20 = rightDir.z;
	result._21 = upDir.z;
	result._22 = forwardDir.z;


	result._30 = -dot(position, rightDir);
	result._31 = -dot(position, upDir);
	result._32 = -dot(position, forwardDir);

	return result;
}


void Camera::getCameraDirections(Vec3 &rightDir, Vec3 &upDir, Vec3 &forwardDir)
{
	Quat cameraRotation = getQuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), yaw);
	cameraRotation = getQuaternionFromAxisAngle(Vec3(1.0f, 0.0f, 0.0f), pitch) * cameraRotation;

	rightDir = rotateVector(Vec3(1.0f, 0.0, 0.0f), cameraRotation);
	upDir = rotateVector(Vec3(0.0, 1.0, 0.0f), cameraRotation);
	forwardDir = rotateVector(Vec3(0.0, 0.0, 1.0f), cameraRotation);
}


void Camera::renderCameraInfo(FontRenderSystem& fontSystem, Vec2 camInfoPosition, const Vec2& fontSize)
{
	char tmpStr[1024];

	snprintf(tmpStr, 1024, "Camera position: (%.2f, %.2f, %.2f), pitch: %.2f, yaw:%.2f", position.x, position.y, position.z, pitch, yaw);
	fontSystem.addText(tmpStr, camInfoPosition, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));


	Vec3 cameraRightdDir;
	Vec3 cameraUpDir;
	Vec3 cameraForwardDir;
	getCameraDirections(cameraRightdDir, cameraUpDir, cameraForwardDir);

	camInfoPosition.y += fontSize.y + 2.0f;

	snprintf(tmpStr, 1024, "Camera look: (%.3f, %.3f, %.3f)", -cameraForwardDir.x, -cameraForwardDir.y, -cameraForwardDir.z);
	fontSystem.addText(tmpStr, camInfoPosition + Vec2(0.0f, fontSize.y) * 0.0f, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	snprintf(tmpStr, 1024, "Camera up: (%.3f, %.3f, %.3f)", cameraUpDir.x, cameraUpDir.y, cameraUpDir.z);
	fontSystem.addText(tmpStr, camInfoPosition + Vec2(0.0f, fontSize.y) * 1.0f, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	snprintf(tmpStr, 1024, "Camera right: (%.3f, %.3f, %.3f)", cameraRightdDir.x, cameraRightdDir.y, cameraRightdDir.z);
	fontSystem.addText(tmpStr, camInfoPosition + Vec2(0.0f, fontSize.y) * 2.0f, fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
}