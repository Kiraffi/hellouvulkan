#include "componentviews.h"

#include <components/transform.h>

#include <imgui/imgui.h>

void drawTransform(Transform &transform)
{
    ImGui::PushID(&transform);
    drawVec3("Pos", transform.pos);
    drawQuat("Rot", transform.rot);
    drawVec3("Scale", transform.scale);
    ImGui::PopID();
}