
#include <app/glfw_keys.h>
#include <app/inputapp.h>
#include <app/vulkan_app.h>

#include <container/podvector.h>
#include <container/string.h>
#include <container/stringview.h>
#include <container/vector.h>

#include <core/general.h>
#include <core/timer.h>
#include <core/mytypes.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkaninitparameters.h>
#include <myvulkan/vulkanresources.h>

#include <render/fontrendersystem.h>

static constexpr i32 c_ScreenWidth = 640;
static constexpr i32 c_ScreenHeight = 540;

static void sUpdateText(StringView str);
static bool sResize();
static void sResized(int width, int height);
static void sHandleInput();
static void sDraw();
static bool sInit();
static void sDeinit();
static void sRenderUpdate();
static void sRunApp();

struct FontRenderData
{
    Vector2 m_fontSize = Vector2(8.0f, 12.0f);
    Image m_renderColorImage;
    String m_text = "Text";
};

static FontRenderData* s_fontRenderData = nullptr;

static void sUpdateText(StringView str)
{
    String tmpStr = "w";
    tmpStr.append(i32(s_fontRenderData->m_fontSize.x));
    tmpStr.append(",h");
    tmpStr.append(i32(s_fontRenderData->m_fontSize.y));

    FontRenderSystem::addText(tmpStr.getStr(),
        Vector2(100.0f, 400.0f),
        s_fontRenderData->m_fontSize,
        Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    FontRenderSystem::addText(String(str.ptr, str.length).getStr(),
        Vector2(100.0f, 100.0f),
        s_fontRenderData->m_fontSize,
        Vector4(1.0f, 1.0f, 1.0f, 1.0f));
}

static bool sResize()
{
    // create color and depth images
    if(!VulkanResources::createRenderTargetImage(vulk->swapchain.width, vulk->swapchain.height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Main color target image", s_fontRenderData->m_renderColorImage))
    {
        printf("Failed to create font render target image!\n");
        return false;
    }
    FontRenderSystem::setRenderTarget(s_fontRenderData->m_renderColorImage);
    return true;
}

static void sResized(int width, int height)
{
    sResize();
}

static void sHandleInput()
{
    auto& text = s_fontRenderData->m_text;
    auto& fontSize = s_fontRenderData->m_fontSize;
    bool textNeedsUpdate = false;

    for(i32 i = 0; i < InputApp::getBufferedInputCount(); ++i)
    {
        text.append(char(InputApp::getBufferedInput(i)));
        textNeedsUpdate = true;
    }

    if (InputApp::isDown(GLFW_KEY_LEFT))
    {
        fontSize.x--;
        if (fontSize.x < 2)
            ++fontSize.x;
        textNeedsUpdate = true;
    }
    if (InputApp::isDown(GLFW_KEY_RIGHT))
    {
        fontSize.x++;
        textNeedsUpdate = true;
    }
    if (InputApp::isDown(GLFW_KEY_DOWN))
    {
        fontSize.y++;
        textNeedsUpdate = true;
    }
    if (InputApp::isDown(GLFW_KEY_UP))
    {
        fontSize.y--;
        if (fontSize.y < 2)
            ++fontSize.y;
        textNeedsUpdate = true;
    }

    sUpdateText(text.getStr());
}

static void sDraw()
{
    auto& image = s_fontRenderData->m_renderColorImage;
    //VulkanResources::flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
    // Should probably abstract this into push images, then
    // then flush barriers for clear image
    {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;
        const VkClearColorValue color{0.0f, 0.0f, 0.0f, 1.0f};
        vulk->imageMemoryGraphicsBarriers.pushBack(VulkanResources::imageBarrier(image,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL));
        VulkanResources::flushBarriers(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);//VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdClearColorImage(vulk->commandBuffer, image.image, image.layout,
            &color, 1, &range);
    }
    {
        VulkanResources::prepareToGraphicsSampleWrite(image);
        FontRenderSystem::render();
    }
    MyVulkan::present(image);
}

static bool sInit(const char* windowStr, i32 screenWidth, i32 screenHeight)
{
    s_fontRenderData = new FontRenderData();
    auto &vulkanInitParams = VulkanInitializationParameters::getRef();
    vulkanInitParams.useIntegratedGpu = true;

    if(!VulkanApp::initApp(windowStr, screenWidth, screenHeight)
        || !InputApp::init()
        || !MyVulkan::init()
        || !FontRenderSystem::init("assets/font/new_font.dat"))
    {
        return false;
    }

    sResize();
    MyVulkan::setVulkanFrameResizedCBFunc(sResized);
    return true;
}

static void sDeinit()
{
    FontRenderSystem::deinit();
    if(s_fontRenderData)
    {
        VulkanResources::destroyImage(s_fontRenderData->m_renderColorImage);
        delete s_fontRenderData;
        s_fontRenderData = nullptr;
    }
    MyVulkan::deinit();

    VulkanApp::deinitApp();
}

static void sRenderUpdate()
{
    vulk->queryPoolIndexCounts[vulk->frameIndex] = 0u;

    //beginSingleTimeCommands();
    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(vulk->commandBuffer, &beginInfo));

    vkCmdResetQueryPool(vulk->commandBuffer, vulk->queryPools[vulk->frameIndex], 0, QUERY_COUNT);

    MyVulkan::writeStamp();
    VulkanResources::update();

    struct FrameBuffer
    {
        Vector2 areaSize;
    };
    FrameBuffer frameBufferData;

    auto& app = VulkanApp::getWindowApp();
    frameBufferData.areaSize = Vec2(app.windowWidth, app.windowHeight);
    VulkanResources::addToCopylist(frameBufferData, vulk->renderFrameBufferHandle[vulk->frameIndex]);

    FontRenderSystem::update();
}

static void sRunApp()
{
    while(VulkanApp::updateApp())
    {
        sHandleInput();

        if (MyVulkan::frameStart())
        {
            //updateStats(*this);
            vulk->currentStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            sRenderUpdate();
            sDraw();
            //renderDraw();
            //printStats(*this);
        }

        defragMemory();
        VulkanApp::frameEnd();
    }
    VK_CHECK(vkDeviceWaitIdle(vulk->device));
}

i32 main(i32 argCount, char **argv)
{
    initMemory();
    if(sInit("Vulkan, render font", c_ScreenWidth, c_ScreenHeight))
    {
        sRunApp();
    }
    sDeinit();
    deinitMemory();
    return 0;
}