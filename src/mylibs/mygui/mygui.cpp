#include "mygui.h"

#include <core/general.h>
#include <container/vector.h>
#include <container/podvector.h>
#include <container/podvectortypedefine.h>

enum GuiObjectType
{
    COLORED,
    TEXT,

    UNKNOWN_GUI_TYPE
};

struct GuiObject
{
    Vec2 pos;
    Vec2 size;
    Vec4 uv;
    u32 col;
    u32 textureIndex;
    GuiObjectType guiObjectType;
    u32 padding;
};

struct GuiWindow
{
    PodVector<GuiObject> guiObjects;
};

struct GuiState
{
    float currentX = 0.0f;
    float currentY = 0.0f;

    Vector<GuiWindow> guiWindows;

    GuiWindow *currentWindows = nullptr;

    const char *previousFocusedGuiWindow = nullptr;
};

static GuiState *globalGuiState = nullptr;

bool initMyGui()
{
    ASSERT(globalGuiState == nullptr);
    globalGuiState = new GuiState;
    return true;
}


void deinitMyGui()
{
    if(globalGuiState)
        delete globalGuiState;
    globalGuiState = nullptr;
    return;
}

void startFrame()
{
    globalGuiState->guiWindows.clear();
}

void endFrame()
{
//    for(const auto &obj : globalGuiState->guiObjects)
//    {
//
//    }
}

void drawGui()
{
//    for(const auto &obj : globalGuiState->guiObjects)
//    {
//
//    }
}

void window(const char *name)
{

}

void guiColoredBox(const Vec2 &pos, const Vec2 &size, const Vec4 &color)
{
    ASSERT(globalGuiState->currentWindows);
    if(globalGuiState->currentWindows == nullptr)
        return;
    globalGuiState->currentWindows->guiObjects.push_back(GuiObject{
        .pos = pos,
        .size = size,
        .uv = Vec4(),
        .col = getColor(color),
        .textureIndex = 0u,
        .guiObjectType = GuiObjectType::COLORED,
        .padding = 0, });
}

void guiText(const Vec2 &pos, const Vec2 &fontSize, const Vec4 &color, const char *txt)
{
    if(!txt)
        return;
    ASSERT(globalGuiState->currentWindows);
    if(globalGuiState->currentWindows == nullptr)
        return;

    u32 i = 0;
    Vec2 renderPos = pos;
    while(txt[i++])
    {
        char c = txt[i];
        if(c == '\n')
        {
            renderPos.x = pos.x;
            renderPos.y += fontSize.y;
            continue;
        }
        if(c < 32)
            continue;

        globalGuiState->currentWindows->guiObjects.push_back(
            GuiObject{
                .pos = renderPos,
                .size = fontSize,
                .uv = Vec4(c - 32, 0, c + 1 - 32, 1),
                .col = getColor(color),
                .textureIndex = 0u,
                .guiObjectType = GuiObjectType::TEXT,
                .padding = 0, }
            );

    }
}