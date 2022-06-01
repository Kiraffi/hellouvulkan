#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/general.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanresources.h>

#include <string>
#include <filesystem>
#include <fstream>

static constexpr int SCREEN_WIDTH = 640;
static constexpr int SCREEN_HEIGHT = 540;


class VulkanFontRender : public VulkanApp
{
public:
    VulkanFontRender() {}
    virtual ~VulkanFontRender() override;

    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

    void updateText(std::string_view str);

private:
    Vector2 fontSize = Vector2(8.0f, 12.0f);
    Image renderColorImage;
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanFontRender::~VulkanFontRender()
{
    destroyImage(renderColorImage);
}

bool VulkanFontRender::init(const char* windowStr, int screenWidth, int screenHeight,
    const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;

    return resized();
}


void VulkanFontRender::updateText(std::string_view str)
{
    std::string tmpStr = "w";
    tmpStr += std::to_string(int32_t(fontSize.x));
    tmpStr += ",h";
    tmpStr += std::to_string(int32_t(fontSize.y));

    fontSystem.addText(tmpStr, Vector2(100.0f, 400.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    fontSystem.addText(str, Vector2(100.0f, 100.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
}

bool VulkanFontRender::resized()
{
    // create color and depth images
    if(!createRenderTargetImage(vulk->swapchain.width, vulk->swapchain.height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Main color target image", renderColorImage))
    {
        printf("Failed to create font render target image!\n");
        return false;
    }
    fontSystem.setRenderTarget(renderColorImage);
    return true;
}

void VulkanFontRender::logicUpdate()
{
    VulkanApp::logicUpdate();
    static std::string txt = "Test";

    {
        bool textNeedsUpdate = false;
        for (int i = 0; i < bufferedPressesCount; ++i)
        {
            txt += char(bufferedPresses[i]);
            textNeedsUpdate = true;
        }

        if (keyDowns[GLFW_KEY_LEFT].isDown)
        {
            fontSize.x--;
            if (fontSize.x < 2)
                ++fontSize.x;
            textNeedsUpdate = true;
        }
        if (keyDowns[GLFW_KEY_RIGHT].isDown)
        {
            fontSize.x++;
            textNeedsUpdate = true;
        }
        if (keyDowns[GLFW_KEY_DOWN].isDown)
        {
            fontSize.y++;
            textNeedsUpdate = true;
        }
        if (keyDowns[GLFW_KEY_UP].isDown)
        {
            fontSize.y--;
            if (fontSize.y < 2)
                ++fontSize.y;
            textNeedsUpdate = true;
        }

        updateText(txt);
    }
}

void VulkanFontRender::renderUpdate()
{
    VulkanApp::renderUpdate();
}

void VulkanFontRender::renderDraw()
{
    addImageBarrier(imageBarrier(renderColorImage,
        0, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

    flushBarriers();
    {
        fontSystem.render();
    }

    writeStamp(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    present(renderColorImage);
}

int main(int argCount, char **argv)
{
    VulkanFontRender app;
    if(app.init("Vulkan, render font", SCREEN_WIDTH, SCREEN_HEIGHT,
        {
            .showInfoMessages = false,
            .useHDR = false,
            .useIntegratedGpu = true,
            .useValidationLayers = true,
            .useVulkanDebugMarkersRenderDoc = false,
            .vsync = VSyncType::IMMEDIATE_NO_VSYNC
        }))
    {
        app.run();
    }

    return 0;
}