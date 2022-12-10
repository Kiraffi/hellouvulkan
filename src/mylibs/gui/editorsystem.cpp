#include "editorsystem.h"

#include <components/transform_functions.h>

#include <app/glfw_keys.h>
#include <app/vulkan_app.h>

#include <gui/componentviews.h>
#include <gui/guiutil.h>

#include <math/hitpoint.h>
#include <math/ray.h>
#include <math/vector3_inline_functions.h>

#include <render/linerendersystem.h>
#include <render/viewport.h>
#include <resources/globalresources.h>

#include <scene/scene.h>


#include <imgui.h>


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



static bool drawEntities(ArraySliceView<GameEntity> gameEntitites, u32 &inOutSelectedEntityIndex)
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
        for(u32 i = 0; i <= u32(EntityType::NUM_OF_ENTITY_TYPES); ++i)
        {
            bool isSelected = i == u32(entity.entityType);
            if(ImGui::Selectable(getStringFromEntityType(EntityType(i)), isSelected))
                entity.entityType = EntityType(i);

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if(isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if(globalResources && u32(entity.entityType) <= globalResources->models.size())
    {
        const auto &model = globalResources->models[u32(entity.entityType)];
        SmallStackString meshName = "";
        if(entity.meshIndex < model.modelMeshes.size())
            meshName = model.modelMeshes[entity.meshIndex].meshName;
        if(ImGui::BeginCombo("Mesh", meshName.getStr(), 0))
        {
            for(u32 i = 0; i < model.modelMeshes.size(); ++i)
            {
                bool isSelected = i == entity.meshIndex;
                if(ImGui::Selectable(model.modelMeshes[i].meshName.getStr()))
                    entity.meshIndex = i;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if(isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        SmallStackString animName = "";
        if(entity.animationIndex < model.animNames.size())
            animName = model.animNames[entity.animationIndex];
        if(entity.animationIndex < model.animNames.size() && ImGui::BeginCombo("Animation", animName.getStr(), 0))
        {
            for(u32 i = 0; i < model.animNames.size(); ++i)
            {
                bool isSelected = i == entity.meshIndex;
                if(ImGui::Selectable(model.animNames[i].getStr()))
                    entity.animationIndex = i;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if(isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }
    ImGui::PopID();
    bool hovered = isWindowHovered();
    return hovered;
}

static bool drawEntity(const char *windowStr, GameEntity &entity)
{
    ImGui::Begin(windowStr);
    bool hover = drawEntityContents(entity);
    ImGui::End();
    return hover;
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
    static const u32 grayColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.75f, 0.75f, 0.75f, 1.0f));
    static const u32 selectedColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.75f, 0.75f, 0.75f, 1.0f));

    static const u32 unSelectedRedColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    static const u32 unSelectedGreenColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    static const u32 unSelectedBlueColor = getColor(Vec4(0.5f, 0.5f, 0.5f, 1.0f) * Vec4(0.0f, 0.0f, 1.0f, 1.0f));

    static const u32 selectedRedColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    static const u32 selectedGreenColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.0f, 1.0f, 0.0f, 1.0f));
    static const u32 selectedBlueColor = getColor(Vec4(1.0f, 1.0f, 1.0f, 1.0f) * Vec4(0.0f, 0.0f, 1.0f, 1.0f));


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

        for(u32 i = 0; i < 8; ++i)
            linePoints[i] = Vec3(linePoints4[i].x, linePoints4[i].y, linePoints4[i].z);

        Vec4 multip(0.5f, 0.5f, 0.5f, 1.0f);
        u32 drawColor = selectedEntityIndex == entity.index ? selectedColor : grayColor;
        lineRenderSystem.addLine(linePoints[1], linePoints[3], drawColor);
        lineRenderSystem.addLine(linePoints[2], linePoints[3], drawColor);
        lineRenderSystem.addLine(linePoints[1], linePoints[5], drawColor);
        lineRenderSystem.addLine(linePoints[2], linePoints[6], drawColor);
        lineRenderSystem.addLine(linePoints[3], linePoints[7], drawColor);
        lineRenderSystem.addLine(linePoints[4], linePoints[5], drawColor);
        lineRenderSystem.addLine(linePoints[4], linePoints[6], drawColor);
        lineRenderSystem.addLine(linePoints[5], linePoints[7], drawColor);
        lineRenderSystem.addLine(linePoints[6], linePoints[7], drawColor);

        u32 redColor = selectedEntityIndex == entity.index ? selectedRedColor : unSelectedRedColor;
        u32 greenColor = selectedEntityIndex == entity.index ? selectedGreenColor : unSelectedGreenColor;
        u32 blueColor = selectedEntityIndex == entity.index ? selectedBlueColor : unSelectedBlueColor;

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


