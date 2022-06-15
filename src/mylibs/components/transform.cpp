#include "transform.h"

#include <core/writejson.h>

bool writeTransform(const Transform &t, WriteJson &json)
{
    json.writeVec3("pos", t.pos);
    json.writeQuat("rot", t.rot);
    json.writeVec3("scale", t.scale);
    return true;
}

