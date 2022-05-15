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

#include <chrono>
#include <string>
#include <thread>
#include <filesystem>
#include <fstream>

static constexpr int SCREEN_WIDTH  = 640;
static constexpr int SCREEN_HEIGHT = 540;

enum TIME_POINTS
{
    START_POINT,
    DRAW_FINISHED,

    NUM_TIME_POINTS
};



class VulkanFontRender : public VulkanApp
{
public:
    VulkanFontRender() {}
    virtual ~VulkanFontRender() override;

    virtual bool init(const char *windowStr, int screenWidth, int screenHeight, 
        const VulkanInitializationParameters &params) override;
    virtual void update() override;
    virtual void resized() override;

    void updateText(std::string& str);

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

bool VulkanFontRender::init(const char *windowStr, int screenWidth, int screenHeight, 
    const VulkanInitializationParameters &params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;

    glfwSetWindowUserPointer(window, this);
    return true;
}


void VulkanFontRender::updateText(std::string &str)
{
    std::string tmpStr = "w";
    tmpStr += std::to_string(int32_t(fontSize.x));
    tmpStr += ",h";
    tmpStr += std::to_string(int32_t(fontSize.y));

    fontSystem.addText(tmpStr, Vector2(100.0f, 400.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    fontSystem.addText(str, Vector2(100.0f, 100.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
}

void VulkanFontRender::resized()
{
    destroyImage(renderColorImage);

    // create color and depth images
    renderColorImage = createImage(
        vulk.swapchain.width, vulk.swapchain.height,
        vulk.defaultColorFormat,

        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main color target image");

}


void VulkanFontRender::update()
{
    VulkanApp::update();
    static std::string txt = "Test";

    ////////////////////////
    //
    // MAIN LOOP START
    // UPDATING ENGINE STATE
    //
    ////////////////////////

    static uint32_t gpuframeCount = 0u;
    static double gpuTime = 0.0;
    static double cpuTimeStamp = glfwGetTime();

    {
        bool textNeedsUpdate = false;
        for (int i = 0; i < bufferedPressesCount; ++i)
        {
            txt += char( bufferedPresses[i] );
            textNeedsUpdate = true;
        }

        if (keyDowns[ GLFW_KEY_LEFT ].isDown)
        {
            fontSize.x--;
            if (fontSize.x < 2)
                ++fontSize.x;
            textNeedsUpdate = true;
        }
        if (keyDowns[ GLFW_KEY_RIGHT ].isDown)
        {
            fontSize.x++;
            textNeedsUpdate = true;
        }
        if (keyDowns[ GLFW_KEY_DOWN ].isDown)
        {
            fontSize.y++;
            textNeedsUpdate = true;
        }
        if (keyDowns[ GLFW_KEY_UP ].isDown)
        {
            fontSize.y--;
            if (fontSize.y < 2)
                ++fontSize.y;
            textNeedsUpdate = true;
        }

        updateText(txt);
    }


    ////////////////////////
    //
    // RENDER PASSES START
    // WRITING VALUES INTO
    // "CONSTANT BUFFEERS"
    //
    ////////////////////////

    if (!startRender())
        return;

    addImageBarrier(imageBarrier(renderColorImage,
        0, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    flushBarriers();

    // Drawingg, just to change background color....
    {
        const SwapChain& swapchain = vulk.swapchain;

        static constexpr VkClearValue colorClear = { .color{48.0f / 255.0f, 10.0f / 255.0f, 36.0f / 255.0f, 1.0f } };
        VkRect2D renderArea = { .extent = {.width = swapchain.width, .height = swapchain.height } };


        const VkRenderingAttachmentInfo colorAttachmentInfo[]{
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .imageView = renderColorImage.imageView,
                .imageLayout = renderColorImage.layout,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = colorClear
            },

        };

        const VkRenderingInfo renderInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea = renderArea,
            .layerCount = 1,
            .colorAttachmentCount = ARRAYSIZES(colorAttachmentInfo),
            .pColorAttachments = colorAttachmentInfo,
            .pDepthAttachment = nullptr,
        };
        vkCmdBeginRendering(vulk.commandBuffer, &renderInfo);

        VkViewport viewPort = { 0.0f, float(vulk.swapchain.height), float(vulk.swapchain.width), -float(vulk.swapchain.height), 0.0f, 1.0f };
        VkRect2D scissors = { { 0, 0 }, { uint32_t(vulk.swapchain.width), uint32_t(vulk.swapchain.height) } };

        insertDebugRegion("Render", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        vkCmdSetViewport(vulk.commandBuffer, 0, 1, &viewPort);
        vkCmdSetScissor(vulk.commandBuffer, 0, 1, &scissors);

        // draw calls here
        // Render
        {
        }
        vkCmdEndRendering(vulk.commandBuffer);

        fontSystem.render(renderColorImage);
    }

    vkCmdWriteTimestamp(vulk.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, vulk.queryPool, TIME_POINTS::DRAW_FINISHED);
    
    present(renderColorImage);

    ////////////////////////
    //
    // END PASS, COLLECT TIMINGS
    //
    ////////////////////////


    uint64_t queryResults[ TIME_POINTS::NUM_TIME_POINTS ];
    vkGetQueryPoolResults(vulk.device, vulk.queryPool, 0, ARRAYSIZES(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[ 0 ]), VK_QUERY_RESULT_64_BIT);


    static double timeDuration[TIME_POINTS::NUM_TIME_POINTS] = {};

    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(vulk.physicalDevice, &props);

    for (uint32_t i = TIME_POINTS::NUM_TIME_POINTS - 1; i > 0; --i)
        timeDuration[ i ] += ( double(queryResults[ i ]) - double(queryResults[ i - 1 ]) ) * props.limits.timestampPeriod * 1.0e-9f;

    gpuTime += ( double(queryResults[ TIME_POINTS::NUM_TIME_POINTS - 1 ]) - double(queryResults[ 0 ]) ) * props.limits.timestampPeriod * 1.0e-9f;

    ++gpuframeCount;
    if (glfwGetTime() - cpuTimeStamp >= 1.0)
    {
        double d = 1000.0 / gpuframeCount;
        double e = gpuframeCount;
        double currTime = glfwGetTime();
        double cpuTime = currTime - cpuTimeStamp;
        cpuTimeStamp += 1.0f;

        printf("Gpu: %.3fms, cpu: %.3fms, draw: %.3fms. GpuFps:%.1f, CpuFps:%.1f\n",
                ( float ) ( gpuTime * d ), ( float ) ( cpuTime * d ),
                ( float ) ( timeDuration[ DRAW_FINISHED ] * d ),
                e / gpuTime, e / cpuTime);
        gpuframeCount = 0u;

        for (uint32_t i = 0; i < TIME_POINTS::NUM_TIME_POINTS; ++i)
            timeDuration[ i ] = 0.0;

        gpuTime = 0.0;
    }
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