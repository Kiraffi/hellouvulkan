#include "componentviews.h"

#include <math/vector3.h>
#include <math/quaternion.h>

#include <imgui/imgui.h>

void drawVec3(const char *name, Vector3 &v)
{
    ImGui::InputFloat3(name, &v.x);
}
void drawQuat(const char *name, Quaternion &q)
{
    ImGui::InputFloat4(name, &q.v.x);
}

