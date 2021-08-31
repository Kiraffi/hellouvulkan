#pragma once

#include "math/matrix.h"
#include "math/quaternion.h"
#include "math/vector3.h"

struct Transform
{
	Vec3 pos;
	Vec3 scale{ 1.0f, 1.0f, 1.0f };
	Quat rot;
};

Matrix getModelMatrix(const Transform &trans)
{
	Matrix posMat = getMatrixFromTranslation(-trans.pos);
	Matrix scaleMat = getMatrixFromScale(trans.scale);
	Quat p = trans.rot;
	p.w = -p.w;
	Matrix rotMat = getMatrixFromQuaternion(p);

	return posMat * rotMat * scaleMat;
}
