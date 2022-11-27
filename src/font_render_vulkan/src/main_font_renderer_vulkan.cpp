
#include <container/podvector.h>
#include <container/string.h>
#include <container/stringview.h>
#include <container/vector.h>

#include <core/general.h>
#include <core/glfw_keys.h>
#include <core/timer.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkaninitparameters.h>
#include <myvulkan/vulkanresources.h>

static constexpr int SCREEN_WIDTH = 640;
static constexpr int SCREEN_HEIGHT = 540;

class VulkanFontRender : public VulkanApp
{
public:
    VulkanFontRender() {}
    virtual ~VulkanFontRender() override;

    virtual bool init(const char* windowStr, int screenWidth, int screenHeight) override;
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

    void updateText(StringView str);

private:
    Vector2 fontSize = Vector2(8.0f, 12.0f);
    Image renderColorImage;
    String text = "Text";
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

bool VulkanFontRender::init(const char* windowStr, int screenWidth, int screenHeight)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight))
        return false;

    return resized();
}


void VulkanFontRender::updateText(StringView str)
{
    String tmpStr = "w";
    tmpStr.append(int32_t(fontSize.x));
    tmpStr.append(",h");
    tmpStr.append(int32_t(fontSize.y));

    fontSystem.addText(tmpStr.getStr(), Vector2(100.0f, 400.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    fontSystem.addText(String(str.ptr, str.length).getStr(), Vector2(100.0f, 100.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
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

    {
        bool textNeedsUpdate = false;
        for (int i = 0; i < bufferedPressesCount; ++i)
        {
            text.append(char(bufferedPresses[i]));
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

        updateText(text.getStr());
    }
}

void VulkanFontRender::renderUpdate()
{
    VulkanApp::renderUpdate();
}

void VulkanFontRender::renderDraw()
{
    VkImageSubresourceRange range;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    const VkClearColorValue color{0.0f, 0.0f, 0.0f, 1.0f};
    vulk->imageMemoryGraphicsBarriers.pushBack(imageBarrier(renderColorImage,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL));
    flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);//VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdClearColorImage(vulk->commandBuffer, renderColorImage.image, renderColorImage.layout,
        &color, 1, &range);

    prepareToGraphicsSampleWrite(renderColorImage);
    fontSystem.render();

    present(renderColorImage);
}

int main(int argCount, char **argv)
{
    initMemory();
    {
        VulkanFontRender app;
        auto &vulkanInitParams = VulkanInitializationParameters::getRef();
        vulkanInitParams.useIntegratedGpu = true;
        if(app.init("Vulkan, render font", SCREEN_WIDTH, SCREEN_HEIGHT))
        {
            app.run();
        }
    }
    deinitMemory();
    return 0;
}