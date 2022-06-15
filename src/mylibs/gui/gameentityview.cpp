#include "componentviews.h"

#include <scene/gameentity.h>

#include <imgui/imgui.h>

bool drawEntity(const char *windowStr, GameEntity &entity)
{
    ImGui::Begin(windowStr);
    bool hover = drawEntityContents(entity);
    ImGui::End();
    return hover;
}

bool drawEntityContents(GameEntity & entity)
{
    ImGui::PushID(&entity);

    char nameStr[32];
    entity.name.copyToCharStr(nameStr, 32);
    ImGui::InputText("Name", nameStr, 32);
    entity.name = nameStr;

    ImGui::Text("Index: %u", entity.index);
    drawTransform(entity.transform);

    const auto *typeName = getStringFromEntityType(entity.entityType);
    if(ImGui::BeginCombo("EntityType", typeName, 0))
    {
        for(uint32_t i = 0; i <= uint32_t(EntityType::NUM_OF_ENTITY_TYPES); ++i)
        {
            bool isSelected = i == uint32_t(entity.entityType);
            if(ImGui::Selectable(getStringFromEntityType(EntityType(i)), isSelected))
                entity.entityType = EntityType(i);

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if(isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::InputInt("Animation index", (int *)&entity.animationIndex);
    
    bool mouseHover = ImGui::IsWindowHovered(
        ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);// | ImGui::IsAnyItemHovered();

    ImGui::PopID();

    return mouseHover;
}