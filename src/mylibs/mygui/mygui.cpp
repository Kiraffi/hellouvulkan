#include "mygui.h"

#include <core/general.h>
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
    uint32_t col;
    uint32_t textureIndex;
    GuiObjectType guiObjectType;
    uint32_t padding;
};



struct GuiState
{
    float currentX = 0.0f;
    float currentY = 0.0f;
    PodVector<GuiObject> guiObjects;
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
    globalGuiState->guiObjects.clear();
}

void drawGui()
{
    for(const auto &obj : globalGuiState->guiObjects)
    {
        
    }
}


void guiColoredBox(const Vec2 &pos, const Vec2 &size, const Vec4 &color)
{
    globalGuiState->guiObjects.push_back(GuiObject{
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

    uint32_t i = 0;
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

        globalGuiState->guiObjects.push_back(
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