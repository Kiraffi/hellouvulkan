#pragma once

#include <container/stackstring.h>
#include <math/vector3.h>

#include <scene/gameentity.h>
#include <render/myimguirenderer.h>
#include <render/viewport.h>


// ImTextureID
#include <imgui.h>

class Scene;
class LineRenderSystem;
class VulkanApp;

struct GLFWwindow;
struct Image;

class EditorSystem
{
public:
    EditorSystem(Scene &scene, LineRenderSystem &lineRenderSystem, u32 windowWidth, u32 windowHeight)
        : scene(scene), lineRenderSystem(lineRenderSystem), editorWindowViewport{ { 0.0f, 0.0f }, { Vec2(windowWidth, windowHeight) } } {}

    ~EditorSystem();

    bool init(GLFWwindow *window);

    bool logicUpdate(const VulkanApp &app);

    bool renderUpdateGui();
    bool renderUpdateViewport();
    bool resizeWindow(const Image &viewportImage);
    Viewport getEditorWindowViewport() const { return editorWindowViewport; }
    bool guiHasFocus() const { return showSaveDialog || !focusOnViewport; }
    bool renderDraw();
    Image &getRenderTargetImage() { return renderTargetImage; }

private:
    Scene &scene;
    LineRenderSystem &lineRenderSystem;

    MyImguiRenderer imguiRenderer;
    SmallStackString levelName = "assets/levels/testmap2.json";

    Image renderTargetImage;

    // This probably should just be type and deducing things from type?
    // On the other hand we are also spawning it to a certain position.
    GameEntity entityToAdd;

    Viewport editorWindowViewport;
    // imgui handle for viewport
    ImTextureID viewportImageHandle = 0;

    // draw hit line for raycasting with mouse
    //Vec3 lineFrom;
    //Vec3 lineTo;

    u32 selectedEntityIndex = ~0u;

    bool showSaveDialog = false;
    bool focusOnViewport = false;
};