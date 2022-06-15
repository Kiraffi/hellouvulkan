#include "quaternion.h"

#include <core/log.h>

void printQuaternion(const Quaternion &q, const char *name)
{
    LOG("%s: %f, %f, %f, %f\n", name, q.v.x, q.v.y, q.v.z, q.w);
}

