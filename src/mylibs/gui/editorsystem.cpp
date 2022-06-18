#include "editorsystem.h"

#include <components/transform_functions.h>

#include <core/glfw_keys.h>
#include <core/vulkan_app.h>

#include <gui/componentviews.h>
#include <gui/guiutil.h>

#include <math/hitpoint.h>
#include <math/ray.h>
#include <math/vector3_inline_functions.h>

#include <render/linerendersystem.h>
#include <render/viewport.h>
#include <scene/scene.h>



#include <imgui/imgui.h>


static bool drawSaveDialog(SmallStackString &inOutName, bool &outSaved)
{
    ImGui::OpenPopup("Save Dialog");

    outSaved = false;
    bool open = true;
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if(ImGui::BeginPopupModal("Save Dialog", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::Text("Save scene: \"%s\"", inOutName.getStr());

        char nameStr[32];
        inOutName.copyToCharStr(nameStr, 32);
        ImGui::InputText("Filename", nameStr, 32);
        inOutName = nameStr;

        ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
        if(ImGui::Button("Save", button_size))
        {
            open = false;
            outSaved = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel", button_size))
        {
            open = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    else
        open = false;

    return open;
}


static bool drawDockspace(ImTextureID viewportImage, Viewport &outViewport, bool &outSaveOpened)
{
    //ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    bool hasFocus = false;

    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    if(dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::Begin("Editor window", nullptr, window_flags);
    {
        ImGui::PopStyleVar(3);
        // Submit the DockSpace
        ImGuiIO &io = ImGui::GetIO();
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

        // Notice menubar drawn on this window.
        if(ImGui::BeginMainMenuBar())
        {
            if(ImGui::BeginMenu("File"))
            {
                if(ImGui::MenuItem("Open", "Ctrl+O")) {}
                if(ImGui::MenuItem("Save", "Ctrl+S")) { outSaveOpened = true; }
                if(ImGui::MenuItem("Save As..")) {}

                ImGui::EndMenu();
            }
            /*
            if(ImGui::BeginMenu("Edit"))
            {
                if(ImGui::MenuItem("Undo", "CTRL+Z")) {}
                if(ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
                ImGui::Separator();
                if(ImGui::MenuItem("Cut", "CTRL+X")) {}
                if(ImGui::MenuItem("Copy", "CTRL+C")) {}
                if(ImGui::MenuItem("Paste", "CTRL+V")) {}
                ImGui::EndMenu();
            }
            */
            ImGui::EndMainMenuBar();
        }
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);

    ImGui::Begin("Viewport", nullptr);//, ImGuiWindowFlags_NoTitleBar);
    {
        Vec2 winSize = Vec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
        winSize = maxVec(Vec2(4.0f, 4.0f), winSize);

        outViewport.pos = Vec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
        outViewport.size = winSize;

        hasFocus = ImGui::IsWindowFocused();
        if(viewportImage)
        {
            ImGui::GetWindowDrawList()->AddImage(
                viewportImage, ImVec2(outViewport.pos.x, outViewport.pos.y),
                ImVec2(outViewport.pos.x + winSize.x, outViewport.pos.y + winSize.y), ImVec2(0, 0), ImVec2(1, 1));
        }

    }

    ImGui::End();
    return hasFocus;
}



static bool drawEntities(ArraySliceView<GameEntity> gameEntitites, uint32_t &inOutSelectedEntityIndex)
{
    ImGui::Begin("Entities");
    if(ImGui::BeginListBox("Entities", ImVec2(-FLT_MIN, 0.0f)))
    {
        for(const auto &entity : gameEntitites)
        {
            char s[256];
            snprintf(s, 256, "Name: %s, Type: %s, index: %u", entity.name.getStr(), getStringFromEntityType(entity.entityType), entity.index);
            const bool isSelected = (inOutSelectedEntityIndex == entity.index);
            if(ImGui::Selectable(s, isSelected))
                inOutSelectedEntityIndex = entity.index;

            if(isSelected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndListBox();
    }

    bool mouseHovered = isWindowHovered();
    ImGui::End();

    return mouseHovered;
}




static bool drawEntity(const char *windowStr, GameEntity &entity)
{
    ImGui::Begin(windowStr);
    bool hover = drawEntityContents(entity);
    ImGui::End();
    return hover;
}

static bool drawEntityContents(GameEntity &entity)
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

    ImGui::PopID();
    bool hovered = isWindowHovered();
    return hovered;
}



static bool drawEntityAddTypes(GameEntity &entityToAdd)
{
    bool addEntityPressed = false;
    ImGui::Begin("Entity types");
    bool focused = drawEntityContents(entityToAdd);
    if(ImGui::Button("Add entity"))
        addEntityPressed = true;

    ImGui::End();

    return addEntityPressed;
}






///////////////////////////////
///
///
///
//////////////////////////////


EditorSystem::~EditorSystem()
{
    destroyImage(renderTargetImage);
}


bool EditorSystem::init(GLFWwindow *window)
{
    if(!window)
        return false;
    if(!imguiRenderer.init(window))
        return false;

    if(!scene.readLevel(levelName.getStr()))
        return false;

    return true;
}

bool EditorSystem::logicUpdate(const VulkanApp &app)
{
    MouseState mouseState = app.getMouseState();

    if(app.isPressed(GLFW_KEY_S) && (app.isDown(GLFW_KEY_LEFT_CONTROL) || app.isDown(GLFW_KEY_RIGHT_CONTROL)))
        showSaveDialog = true;

    if(!guiHasFocus())
    {

        if(mouseState.leftButtonDown && focusOnViewport &&
            mouseState.x >= editorWindowViewport.pos.x && mouseState.y >= editorWindowViewport.pos.y &&
            mouseState.x < editorWindowViewport.pos.x + editorWindowViewport.size.x &&
            mouseState.y < editorWindowViewport.pos.y + editorWindowViewport.size.y)
        {
            selectedEntityIndex = ~0u;
            Vec2 coord = Vec2(mouseState.x, mouseState.y) - editorWindowViewport.pos;
            Ray ray = app.getActiveCamera().getRayFromScreenPixelCoordinates(coord, editorWindowViewport.size);

            HitPoint hitpoint{ Uninit };
            selectedEntityIndex = scene.castRay(ray, hitpoint);
            if(selectedEntityIndex != ~0u)
            {
                //lineFrom = ray.pos;
                //lineTo = hitpoint.point;
                entityToAdd.transform.pos = hitpoint.point;
            }
        }
    }
    return true;
}


bool EditorSystem::renderUpdateViewport()
{
    static const uint32_t grayColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.75f, 0.75f, 0.75f, 1.0f));
    static const uint32_t selectedColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.75f, 0.75f, 0.75f, 1.0f));

    static const uint32_t unSelectedRedColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    static const uint32_t unSelectedGreenColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    static const uint32_t unSelectedBlueColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.0f, 0.0f, 1.0f, 1.0f));

    static const uint32_t selectedRedColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    static const uint32_t selectedGreenColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    static const uint32_t selectedBlueColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.0f, 0.0f, 1.0f, 1.0f));


    for(const auto &entity : scene.getEntities())
    {
        Vec4 linePoints4[8];
        Vec3 linePoints[8];
        Mat3x4 m = getModelMatrix(entity.transform);
        const auto &bounds = scene.getBounds(entity.index);
        const auto &bmin = bounds.min;
        const auto &bmax = bounds.max;

        linePoints4[0] = mul(m, Vec4(bmin.x, bmin.y, bmin.z, 1.0f));
        linePoints4[1] = mul(m, Vec4(bmax.x, bmin.y, bmin.z, 1.0f));
        linePoints4[2] = mul(m, Vec4(bmin.x, bmax.y, bmin.z, 1.0f));
        linePoints4[3] = mul(m, Vec4(bmax.x, bmax.y, bmin.z, 1.0f));
        linePoints4[4] = mul(m, Vec4(bmin.x, bmin.y, bmax.z, 1.0f));
        linePoints4[5] = mul(m, Vec4(bmax.x, bmin.y, bmax.z, 1.0f));
        linePoints4[6] = mul(m, Vec4(bmin.x, bmax.y, bmax.z, 1.0f));
        linePoints4[7] = mul(m, Vec4(bmax.x, bmax.y, bmax.z, 1.0f));

        for(uint32_t i = 0; i < 8; ++i)
            linePoints[i] = Vec3(linePoints4[i].x, linePoints4[i].y, linePoints4[i].z);

        Vec4 multip(0.5f, 0.5f, 0.5f, 1.0f);
        uint32_t drawColor = selectedEntityIndex == entity.index ? selectedColor : grayColor;
        lineRenderSystem.addLine(linePoints[1], linePoints[3], drawColor);
        lineRenderSystem.addLine(linePoints[2], linePoints[3], drawColor);
        lineRenderSystem.addLine(linePoints[1], linePoints[5], drawColor);
        lineRenderSystem.addLine(linePoints[2], linePoints[6], drawColor);
        lineRenderSystem.addLine(linePoints[3], linePoints[7], drawColor);
        lineRenderSystem.addLine(linePoints[4], linePoints[5], drawColor);
        lineRenderSystem.addLine(linePoints[4], linePoints[6], drawColor);
        lineRenderSystem.addLine(linePoints[5], linePoints[7], drawColor);
        lineRenderSystem.addLine(linePoints[6], linePoints[7], drawColor);

        uint32_t redColor = selectedEntityIndex == entity.index ? selectedRedColor : unSelectedRedColor;
        uint32_t greenColor = selectedEntityIndex == entity.index ? selectedGreenColor : unSelectedGreenColor;
        uint32_t blueColor = selectedEntityIndex == entity.index ? selectedBlueColor : unSelectedBlueColor;

        lineRenderSystem.addLine(linePoints[0], linePoints[1], redColor);
        lineRenderSystem.addLine(linePoints[0], linePoints[2], greenColor);
        lineRenderSystem.addLine(linePoints[0], linePoints[4], blueColor);

        if(entity.entityType == EntityType::NUM_OF_ENTITY_TYPES ||
            entity.entityType == EntityType::FLOOR)
            continue;
    }
    //lineRenderSystem.addLine(lineFrom, lineTo, getColor(0.0f, 1.0f, 0.0f, 1.0f));

    return true;
}

bool EditorSystem::renderUpdateGui()
{
    imguiRenderer.renderBegin();

    focusOnViewport = drawDockspace(viewportImageHandle, editorWindowViewport, showSaveDialog);


    drawEntities(sliceFromPodVector(scene.getEntities()), selectedEntityIndex);
    if(drawEntityAddTypes(entityToAdd))
    {
        focusOnViewport = false;
        selectedEntityIndex = scene.addGameEntity(entityToAdd);
    }
    
    bool saved = false;

    if(showSaveDialog && !drawSaveDialog(levelName, saved))
    {
        showSaveDialog = false;
        if(saved)
            scene.writeLevel(levelName.getStr());
    }
    {
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
        ImGui::Begin("Properties");
        if(selectedEntityIndex != ~0u)
            drawEntityContents(scene.getEntity(selectedEntityIndex));
        ImGui::End();
    }

    return true;
}

bool EditorSystem::resizeWindow(const Image &viewportImage)
{
    // create color and depth images
    if(!createRenderTargetImage(vulk->swapchain.width, vulk->swapchain.height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        "Imgui render target", renderTargetImage))
    {
        printf("Failed to create %s\n", renderTargetImage.imageName);
        return false;
    }

    MyImguiRenderer::addTexture(viewportImage, (VkDescriptorSet &)viewportImageHandle);

    return imguiRenderer.updateRenderTarget(renderTargetImage);
}

bool EditorSystem::renderDraw()
{
    prepareToGraphicsSampleWrite(renderTargetImage);
    flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    imguiRenderer.render();
    return true;
}


