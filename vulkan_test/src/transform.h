#pragma once

#include <math/matrix.h>
#include <math/quaternion.h>
#include <math/vector3.h>

struct Transform
{
	Vec3 pos;
	Vec3 scale;
	Quat rot;
};

Matrix getModelMatrix(const Transform &trans)
{
	Matrix posMat = getMatrixFromTranslation(trans.pos);
	Matrix scaleMat = getMatrixFromScale(trans.scale);
	Matrix rotMat = getMatrixFromQuaternion(trans.rot);

	return posMat * rotMat * scaleMat;
}
