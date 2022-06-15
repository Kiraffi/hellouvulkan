#pragma once

#include <math/matrix.h>
#include <math/quaternion.h>
#include <math/vector3.h>

class WriteJson;

struct Transform
{
    Vec3 pos;
    Quat rot;
    Vec3 scale{ 1.0f, 1.0f, 1.0f };
};

bool writeTransform(const Transform &t, WriteJson &json);