
#include <app/glfw_keys.h>
#include <app/inputapp.h>
#include <app/vulkan_app.h>

#include <container/podvector.h>
#include <container/podvectortypedefine.h>
#include <container/string.h>
#include <container/vector.h>

#include <core/file.h>
#include <core/general.h>
#include <core/mytypes.h>
#include <core/timer.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>
#include <math/vector3_inline_functions.h>

#include <render/fontrendersystem.h>
#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

static constexpr i32 c_ScreenWidth  = 640;
static constexpr i32 c_ScreenHeight = 540;

static constexpr VkDeviceSize c_QuadBufferSize = 8 * 1024 * 1024u;

static constexpr float c_ButtonSize = 20.0f;
static constexpr float c_SmallButtonSize = 2.0f;
static constexpr float c_BorderSizes = 2.0f;

static constexpr u32 c_LetterStartIndex = 32;
static constexpr u32 c_LetterEndIndex = 128;
static constexpr u32 c_LetterCount = c_LetterEndIndex - c_LetterStartIndex;

static constexpr u32 c_LetterWidth = 8;
static constexpr u32 c_LetterHeight = 12;

static constexpr float c_LetterPanelOffsetX = 10.0f;
static constexpr float c_LetterPanelOffsetY = 10.0f;

static constexpr VkClearValue c_ColorClear = { .color{ 0.0f, 0.5f, 1.0f, 1.0f } };

static void sRunApp();
static void sHandleInput();
static void sDraw();
static void sRenderUpdate();

static bool sInit(const char *windowStr, i32 screenWidth, i32 screenHeight, const char* fontName);
static bool sInitRun();
static void sDeinit();
static bool sResize();
static void sResized(int width, int height);

static void sUpdateCharacterImageData(u32 characterIndex);
static void sUpdateDrawAreaData();
static Vec2 sGetDrawAreaStartPos();

struct GPUVertexData
{
    float m_posX;
    float m_posY;
    u16 m_pixelSizeX;
    u16 m_pixelSizeY;
    u32 m_color;
};

struct Box
{
    Vec2 m_pos;
    Vec2 m_size;
};

struct VulkanFontDrawData
{
    Image m_renderColorImage;

    Buffer m_quadBuffer[VulkanGlobal::FramesInFlight];
    Buffer m_indexDataBuffer;

    Pipeline m_graphicsPipeline;

    String m_fontFilename;

    u8 m_buffData[12] = {};
    i32 m_chosenLetter = 'a';

    // index 0 = selected box, 1..letters * lettersize = letter pixels, after that drawing area
    PodVector<GPUVertexData> m_vertData;
    PodVector<u8> m_characterData;

    PodVector<Box> m_charactersInScreen;
};

static VulkanFontDrawData *s_data = nullptr;

i32 main(i32 argCount, char **argv)
{
    initMemory();
    const char* filename;
    if (argCount < 2)
    {
        filename = "assets/font/new_font.dat";
    }
    else
    {
        filename = argv[ 1 ];
    }
    if(sInit("Vulkan, draw font", c_ScreenWidth, c_ScreenHeight, filename))
    {
        sRunApp();
    }
    sDeinit();
    deinitMemory();
    return 0;
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

static void sHandleInput()
{
    //VulkanApp::logicUpdate();

    MouseState mouseState = InputApp::getMouseState();
    {
        if (InputApp::isPressed(GLFW_KEY_LEFT))
            --s_data->m_chosenLetter;

        if (InputApp::isPressed(GLFW_KEY_RIGHT))
            ++s_data->m_chosenLetter;

        if (InputApp::isPressed(GLFW_KEY_DOWN))
            s_data->m_chosenLetter += 8;

        if (InputApp::isPressed(GLFW_KEY_UP))
            s_data->m_chosenLetter -= 8;

        bool isControlDown =
            InputApp::isDown(GLFW_KEY_LEFT_CONTROL) | InputApp::isDown(GLFW_KEY_RIGHT_CONTROL);

        for (i32 i = 0; i < InputApp::getBufferedInputCount(); ++i)
        {
            u32 key = InputApp::getBufferedInput(i);
            if (!isControlDown && key >=  c_LetterStartIndex && key < c_LetterEndIndex)
            {
                s_data->m_chosenLetter = (int)key;
            }
        }

        if (s_data->m_chosenLetter < c_LetterStartIndex)
            s_data->m_chosenLetter = c_LetterStartIndex;
        if (s_data->m_chosenLetter >= c_LetterEndIndex)
            s_data->m_chosenLetter = c_LetterEndIndex - 1;


        if (isControlDown && InputApp::isDown(GLFW_KEY_S))
            writeBytes(s_data->m_fontFilename.getStr(), s_data->m_characterData.getBuffer());

        if (isControlDown && InputApp::isDown(GLFW_KEY_L))
        {
            loadBytes(s_data->m_fontFilename.getStr(), s_data->m_characterData.getBuffer());
            for(u32 i  = 0; i < c_LetterCount; ++i)
                sUpdateCharacterImageData(i + c_LetterStartIndex);
        }

        if (isControlDown && InputApp::isDown(GLFW_KEY_C))
        {
            u32 ind = (s_data->m_chosenLetter - c_LetterStartIndex) * c_LetterHeight;
            u8 *ptr = &s_data->m_characterData[ind];
            for (i32 i = 0; i < c_LetterHeight; ++i)
            {
                s_data->m_buffData[i] = ptr[i];
            }
        }

        if (isControlDown && InputApp::isDown(GLFW_KEY_V))
        {
            u32 ind = (s_data->m_chosenLetter - c_LetterStartIndex) * c_LetterHeight;
            u8 *ptr = &s_data->m_characterData[ind];
            for (i32 i = 0; i < c_LetterHeight; ++i)
            {
                ptr[i] = s_data->m_buffData[i];
            }
        }
        // check if we click any of the characters?
        if(mouseState.leftButtonDown)
        {
            for(u32 i = 0; i < s_data->m_charactersInScreen.size(); ++i)
            {
                const auto &box = s_data->m_charactersInScreen[i];
                if(mouseState.x >= box.m_pos.x && mouseState.x <= box.m_pos.x + box.m_size.x &&
                    mouseState.y >= box.m_pos.y && mouseState.y <= box.m_pos.y + box.m_size.y)
                {
                    s_data->m_chosenLetter = c_LetterStartIndex + i;
                    break;
                }
            }
        }
        // Check if drawing or erasing font letter pixels
        if(mouseState.leftButtonDown || mouseState.rightButtonDown)
        {
            Vec2 drawBox = Vec2(mouseState.x, mouseState.y) - sGetDrawAreaStartPos();

            i32 foundI = drawBox.x / ((c_BorderSizes + c_ButtonSize));
            i32 foundJ = drawBox.y / ((c_BorderSizes + c_ButtonSize));
            // -0.1 / 1 = 0... so cannot check if foundI >= 0
            if(drawBox.x >= 0.0f && foundI < c_LetterWidth
                && drawBox.y >= 0.0f && foundJ < c_LetterHeight)
            {
                u8 *visiblePtr =
                    &s_data->m_characterData[(s_data->m_chosenLetter - c_LetterStartIndex) * c_LetterHeight];
                u8 &visibleRow = visiblePtr[foundJ];
                u8 bit = (1 << foundI);
                // set bit
                if (mouseState.leftButtonDown)
                    visibleRow |= bit;
                // remove bit
                else if (mouseState.rightButtonDown)
                    visibleRow &= ~(bit);
            }
        }
        sUpdateCharacterImageData(s_data->m_chosenLetter);
        sUpdateDrawAreaData();

        u32 xOff = ((s_data->m_chosenLetter - c_LetterStartIndex) % c_LetterWidth);
        u32 yOff = ((s_data->m_chosenLetter - c_LetterStartIndex) / c_LetterWidth);

        s_data->m_vertData[0].m_posX = c_LetterPanelOffsetX + (xOff * c_LetterWidth ) * c_SmallButtonSize + xOff * 2 - 2;
        s_data->m_vertData[0].m_posY = c_LetterPanelOffsetY + (yOff * c_LetterHeight) * c_SmallButtonSize + yOff * 2 - 2;
    }

    ASSERT(s_data->m_vertData.size() * sizeof(GPUVertexData) < c_QuadBufferSize);
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
        Matrix camMat;
        Matrix viewProj;
        Matrix mvp;
        Matrix sunMatrix;

        Vector2 areaSize;
        Vector2 tmp1;

        Vector4 camPos;
        Vector4 tmp3;
        Vector4 tmp4;

        Matrix inverseMvp;
    };
    FrameBuffer frameBufferData;
/*
    // bit ugly.......
    sunCamera.calculateOrtographicPosition(camera.position);
    if(windowWidth > 0 && windowHeight > 0)
        camera.updateCameraState(windowWidth, windowHeight);
    sunCamera.updateCameraState(50.0f, 50.0f);

    if (useSunCamera)
    {
        frameBufferData.mvp = sunCamera.worldToViewMat;
        frameBufferData.inverseMvp = sunCamera.viewToWorldMat;
    }
    else
    {
        frameBufferData.mvp = camera.worldToViewMat;
        frameBufferData.inverseMvp = camera.viewToWorldMat;
    }
    frameBufferData.sunMatrix = sunCamera.worldToViewMat;
    frameBufferData.camPos = Vector4(camera.position, 0.0f);
*/
    auto& app = VulkanApp::getWindowApp();
    frameBufferData.areaSize = Vec2(app.windowWidth, app.windowHeight);
    VulkanResources::addToCopylist(frameBufferData, vulk->renderFrameBufferHandle[vulk->frameIndex]);

    FontRenderSystem::update();

    VulkanResources::addToCopylist(
        sliceFromPodVectorBytes(s_data->m_vertData), s_data->m_quadBuffer[vulk->frameIndex]);
}

static void sDraw()
{
    VulkanResources::prepareToGraphicsSampleWrite(s_data->m_renderColorImage);

    // Drawingg

    {

        MyVulkan::beginRenderPass(s_data->m_graphicsPipeline, { c_ColorClear });
        // draw calls here
        // Render
        {
            MyVulkan::bindGraphicsPipelineWithDecriptors(s_data->m_graphicsPipeline, vulk->frameIndex);
            vkCmdBindIndexBuffer(vulk->commandBuffer, s_data->m_indexDataBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulk->commandBuffer, u32(s_data->m_vertData.size() * 6), 1, 0, 0, 0);

        }
        vkCmdEndRenderPass(vulk->commandBuffer);
        //vkCmdEndRendering(vulk->commandBuffer);
    }

    MyVulkan::writeStamp();

    MyVulkan::present(s_data->m_renderColorImage);
}

static bool sInit(const char *windowStr, i32 screenWidth, i32 screenHeight, const char* fontName)
{
    s_data = new VulkanFontDrawData();
    s_data->m_fontFilename = fontName;
    if (!VulkanApp::initApp("Vulkan, draw font", c_ScreenWidth, c_ScreenHeight)
            || !InputApp::init()
        || !MyVulkan::init()
        || !FontRenderSystem::init(fontName))
    {
        return false;
    }
    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
    {
        s_data->m_quadBuffer[i] = VulkanResources::createBuffer(c_QuadBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Quad buffer");
    }
    s_data->m_indexDataBuffer = VulkanResources::createBuffer(32 * 1024 * 1024,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");

    // Random tag data
    //struct DemoTag { const char name[17] = "debug marker tag"; } demoTag;
    //setObjectTag(device, (u64)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);


    {
        u32 offset = 0;
        PodVector<u32> indices;
        indices.resize(6 * 10240);
        for (i32 i = 0; i < 10240; ++i)
        {
            indices[ size_t(i) * 6 + 0 ] = i * 4 + 1;
            indices[ size_t(i) * 6 + 1 ] = i * 4 + 0;
            indices[ size_t(i) * 6 + 2 ] = i * 4 + 2;

            indices[ size_t(i) * 6 + 3 ] = i * 4 + 2;
            indices[ size_t(i) * 6 + 4 ] = i * 4 + 0;
            indices[ size_t(i) * 6 + 5 ] = i * 4 + 3;
        }
        offset = VulkanResources::uploadToScratchbuffer(
            ( void * ) indices.data(),
            size_t(sizeof(indices[ 0 ]) * indices.size()),
            offset);
        VulkanResources::uploadScratchBufferToGpuBuffer(s_data->m_indexDataBuffer, offset);
    }

    {
        Pipeline& pipeline = s_data->m_graphicsPipeline;
        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);


        pipeline.renderPass = MyVulkan::createRenderPass(
            { RenderTarget{ .format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR } },
            {});
        ASSERT(pipeline.renderPass);
        if(!pipeline.renderPass)
            return false;

        VkPipelineColorBlendAttachmentState rgbaAtt{ .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        if (!MyVulkan::createGraphicsPipeline(
            VulkanShader::getShader(ShaderType::ColoredQuadVert), VulkanShader::getShader(ShaderType::ColoredQuadFrag),
            { { rgbaAtt } }, {}, pipeline, "Font draw render"))
        {
            printf("Failed to create pipeline\n");
            return false;
        }

        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),
                DescriptorInfo(s_data->m_quadBuffer[i]),
            };
        }
        if (!VulkanShader::updateBindDescriptorSet(pipeline))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }

    sResize();
    MyVulkan::setVulkanFrameResizedCBFunc(sResized);
    return sInitRun();
}

static bool sInitRun()
{
    if (!loadBytes(s_data->m_fontFilename.getStr(), s_data->m_characterData.getBuffer()))
    {
        printf("Failed to load file: %s\n", s_data->m_fontFilename.getStr());
        return false;
    }
    s_data->m_charactersInScreen.resize(c_LetterCount);

    s_data->m_vertData.resize(c_LetterHeight * c_LetterWidth * (c_LetterCount + 1) + 1);


    // selected red box
    {
        auto& app = VulkanApp::getWindowApp();

        float offX = (c_BorderSizes + c_ButtonSize) + app.windowWidth * 0.5f;
        float offY = (c_BorderSizes + c_ButtonSize) + app.windowHeight * 0.5f;

        GPUVertexData& vdata = s_data->m_vertData[0];
        vdata.m_color = getColor(1.0f, 0.0f, 0.0f, 1.0f);
        vdata.m_pixelSizeX = u16(c_SmallButtonSize) * c_LetterWidth + 4;
        vdata.m_pixelSizeY = u16(c_SmallButtonSize) * c_LetterHeight + 4;
        vdata.m_posX = offX;
        vdata.m_posY = offY;
    }

    // Draw area boxes
    {
        sUpdateDrawAreaData();
    }
    for (i32 k = 0; k < c_LetterCount; ++k)
    {
        i32 x = k % c_LetterWidth;
        i32 y = k / c_LetterWidth;
        float smallOffX =
            c_LetterPanelOffsetX + float(x * c_LetterWidth) * c_SmallButtonSize + x * 2;
        float smallOffY =
            c_LetterPanelOffsetY + float(y * c_LetterHeight) * c_SmallButtonSize + y * 2;

        s_data->m_charactersInScreen[k].m_pos = Vec2(smallOffX, smallOffY);
        s_data->m_charactersInScreen[k].m_size = Vec2(c_LetterWidth * c_SmallButtonSize,
            c_LetterHeight * c_SmallButtonSize);

        sUpdateCharacterImageData(k +  c_LetterStartIndex);
    }
    return true;
}

static void sDeinit()
{
    FontRenderSystem::deinit();
    if(s_data)
    {
        MyVulkan::destroyPipeline(s_data->m_graphicsPipeline);

        VulkanResources::destroyImage(s_data->m_renderColorImage);
        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
            VulkanResources::destroyBuffer(s_data->m_quadBuffer[i]);
        VulkanResources::destroyBuffer(s_data->m_indexDataBuffer);
        delete s_data;
        s_data = nullptr;
    }

    MyVulkan::deinit();
    VulkanApp::deinitApp();
}

static bool sResize()
{
    VulkanResources::destroyImage(s_data->m_renderColorImage);

    // create color and depth images
    if (!VulkanResources::createRenderTargetImage(
        vulk->swapchain.width, vulk->swapchain.height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Main color target image", s_data->m_renderColorImage))
    {
        printf("Failed to create color image\n");
        return false;
    }

    ASSERT(VulkanResources::createFramebuffer(s_data->m_graphicsPipeline, { s_data->m_renderColorImage }));
    FontRenderSystem::setRenderTarget(s_data->m_renderColorImage);

    return true;
}

static void sResized(int width, int height)
{
    sResize();
}

static void sUpdateCharacterImageData(u32 characterIndex)
{
    ASSERT(characterIndex >= c_LetterStartIndex && characterIndex < c_LetterEndIndex);
    characterIndex -=  c_LetterStartIndex;

    const Vec2 startOffset = s_data->m_charactersInScreen[characterIndex].m_pos;
    const Vec2 size = s_data->m_charactersInScreen[characterIndex].m_size / Vec2(c_LetterWidth, c_LetterHeight);

    u32 startIndex = characterIndex * c_LetterHeight;
    u8 *visibility = (u8 *)&s_data->m_characterData[startIndex];

    // first LetterWidth * LetterHeight is reserved for the drawing middle.
    GPUVertexData *vertDataPtr = &s_data->m_vertData[(characterIndex + 1) * c_LetterWidth * c_LetterHeight + 1];

    for (u32 j = 0; j < c_LetterHeight; ++j)
    {
        u8 rowVisibility = visibility[j];
        for (u32 i = 0; i < c_LetterWidth; ++i)
        {
            bool isVisible = ((rowVisibility >> i) & 1) == 1;

            vertDataPtr->m_color = isVisible ? ~0u : 0u;
            vertDataPtr->m_posX = u16(startOffset.x + i * c_SmallButtonSize);
            vertDataPtr->m_posY = u16(startOffset.y + j * c_SmallButtonSize);
            vertDataPtr->m_pixelSizeX = u16(size.x);
            vertDataPtr->m_pixelSizeY = u16(size.y);
            ++vertDataPtr;
        }

    }

}

static void sUpdateDrawAreaData()
{
    ASSERT(s_data->m_chosenLetter >=  c_LetterStartIndex && s_data->m_chosenLetter <  c_LetterEndIndex);
    u32 startIndex = (s_data->m_chosenLetter - c_LetterStartIndex) * c_LetterHeight;
    u8 *visibility = (u8 *)&s_data->m_characterData[startIndex];

        // Draw area boxes
    Vec2 drawStart = sGetDrawAreaStartPos();
    for (i32 j = 0; j < c_LetterHeight; ++j)
    {
        u8 visibilityBits = visibility[j];
        for (i32 i = 0; i < c_LetterWidth; ++i)
        {
            Vec2 pos = drawStart + Vec2(i, j) * (c_BorderSizes + c_ButtonSize);
            GPUVertexData& vdata = s_data->m_vertData[i + size_t(j) * c_LetterWidth + 1];
            vdata.m_color = ((visibilityBits >> i) & 1) ? ~0u : 0u;
            vdata.m_pixelSizeX = vdata.m_pixelSizeY = c_ButtonSize;
            vdata.m_posX = pos.x;
            vdata.m_posY = pos.y;
        }
    }
}

static Vec2 sGetDrawAreaStartPos()
{
    auto& app = VulkanApp::getWindowApp();

    return Vec2(
        float(c_LetterWidth * -0.5f) * (c_BorderSizes + c_ButtonSize) + app.windowWidth * 0.5f,
        float(c_LetterHeight * -0.5f) * (c_BorderSizes + c_ButtonSize) + app.windowHeight * 0.5f);
}

