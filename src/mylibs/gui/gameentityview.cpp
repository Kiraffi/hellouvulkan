#include "componentviews.h"

#include <scene/gameentity.h>

#include <imgui/imgui.h>

bool drawEntity(GameEntity &entity)
{
    ImGui::Begin("Entity");
    ImGui::Text("Type: %u, index: %u", uint32_t(entity.entityType), entity.index);
    drawTransform(entity.transform);
    bool mouseHover = ImGui::IsWindowHovered(
        ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);// | ImGui::IsAnyItemHovered();
    ImGui::End();

    return mouseHover;
}