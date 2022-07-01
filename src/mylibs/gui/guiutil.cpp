#include "guiutil.h"

#include <imgui.h>

bool isWindowHovered()
{
    bool mouseHover = ImGui::IsWindowHovered(
        ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);// | ImGui::IsAnyItemHovered();
    return mouseHover;
}