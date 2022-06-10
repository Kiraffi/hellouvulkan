#pragma once

#include <math/matrix.h>
#include <math/quaternion.h>
#include <math/vector3.h>

struct Transform
{
    Vec3 pos;
    Quat rot;
    Vec3 scale{ 1.0f, 1.0f, 1.0f };
};

static Matrix getModelMatrix(const Transform &trans)
{
    Matrix posMat = getMatrixFromTranslation(trans.pos);
    Matrix scaleMat = getMatrixFromScale(trans.scale);
    Matrix rotMat = getMatrixFromQuaternion(trans.rot);
    Matrix result = posMat * rotMat * scaleMat;
    return result;
}

static Matrix getModelMatrixInverse(const Transform &trans)
{
    Matrix posMat = getMatrixFromTranslation(-trans.pos);
    Vec3 scale = trans.scale;
    scale.x = 1.0f / scale.x;
    scale.y = 1.0f / scale.y;
    scale.z = 1.0f / scale.z;
    Quat rot = trans.rot;
    rot.v = -rot.v;
    Matrix scaleMat = getMatrixFromScale(scale);
    Matrix rotMat = getMatrixFromQuaternion(rot);

    return scaleMat * rotMat * posMat;
}
