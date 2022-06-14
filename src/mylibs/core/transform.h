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

static Mat3x4 getModelMatrix(const Transform &trans)
{
    Mat3x4 posMat = getMatrixFromTranslation(trans.pos);
    Mat3x4 scaleMat = getMatrixFromScale(trans.scale);
    Mat3x4 rotMat = getMatrixFromQuaternion(trans.rot);
    Mat3x4 result = posMat * rotMat * scaleMat;
    return result;
}

static Mat3x4 getModelMatrixInverse(const Transform &trans)
{
    Mat3x4 posMat = getMatrixFromTranslation(-trans.pos);
    Vec3 scale = trans.scale;
    scale.x = 1.0f / scale.x;
    scale.y = 1.0f / scale.y;
    scale.z = 1.0f / scale.z;
    Quat rot = trans.rot;
    rot.v = -rot.v;
    Mat3x4 scaleMat = getMatrixFromScale(scale);
    Mat3x4 rotMat = getMatrixFromQuaternion(rot);

    return scaleMat * rotMat * posMat;
}
