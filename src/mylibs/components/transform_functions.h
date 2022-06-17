#pragma once

#include <components/transform.h>
#include <math/matrix_inline_functions.h>

static Mat3x4 getModelMatrix(const Transform &trans)
{
    //Mat3x4 posMat = getMatrixFromTranslation(trans.pos);
    //Mat3x4 scaleMat = getMatrixFromScale(trans.scale);
    //Mat3x4 rotMat = getMatrixFromQuaternion(trans.rot);
    //Mat3x4 result = posMat * rotMat * scaleMat;
    //return result;
    return getMatrixFromTranslation(trans);
}

static Mat3x4 getModelMatrixInverse(const Transform &trans)
{
    /*
    Vec3 scale = trans.scale;
    scale.x = 1.0f / scale.x;
    scale.y = 1.0f / scale.y;
    scale.z = 1.0f / scale.z;
    Quat rot(-trans.rot.v.x, -trans.rot.v.y, -trans.rot.v.z, trans.rot.w);
    Vec3 pos(-trans.pos.x, -trans.pos.y, -trans.pos.z);
    Mat3x4 posMat = getMatrixFromTranslation(pos);
    Mat3x4 scaleMat = getMatrixFromScale(scale);
    Mat3x4 rotMat = getMatrixFromQuaternion(rot);
    
    return scaleMat * rotMat * posMat;
    */

    return getInverseMatrixFromTranslation(trans);
}

static Mat3x4 getModelNormalMatrix(const Transform &trans)
{
    Vec3 scale = trans.scale;
    scale.x = 1.0f / scale.x;
    scale.y = 1.0f / scale.y;
    scale.z = 1.0f / scale.z;
    //Mat3x4 scaleMat = getMatrixFromScale(scale);
    //Mat3x4 rotMat = getMatrixFromQuaternion(trans.rot);
    //
    //return rotMat * scaleMat;

    return getMatrixFromTranslation({ .pos = {0, 0, 0}, .rot = trans.rot, .scale = scale });
}
