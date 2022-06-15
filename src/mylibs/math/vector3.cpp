
#include "vector3.h"

#include <core/log.h>

void printVector3(const Vector3 &v, const char *name)
{
    LOG("%s: %f, %f, %f\n", name, v.x, v.y, v.z);
}

