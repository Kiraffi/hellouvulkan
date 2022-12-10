
#include <container/podvector.h>
#include <container/podvectortypedefine.h>
#include <container/string.h>
#include <container/vector.h>

#include <core/file.h>
#include <app/glfw_keys.h>
#include <core/general.h>
#include <core/mytypes.h>
#include <core/timer.h>
#include <app/vulkan_app.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>
#include <math/vector3_inline_functions.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

static constexpr i32 SCREEN_WIDTH  = 640;
static constexpr i32 SCREEN_HEIGHT = 540;

static constexpr VkDeviceSize QuadBufferSize = 8 * 1024 * 1024u;

static constexpr float buttonSize = 20.0f;
static constexpr float smallButtonSize = 2.0f;
static constexpr float borderSizes = 2.0f;

static constexpr u32  LetterStartIndex = 32;
static constexpr u32  LetterEndIndex = 128;
static constexpr u32 LetterCount =  LetterEndIndex -  LetterStartIndex;

static constexpr u32 LetterWidth = 8;
static constexpr u32 LetterHeight = 12;

static constexpr float LetterPanelOffsetX = 10.0f;
static constexpr float LetterPanelOffsetY = 10.0f;

struct GPUVertexData
{
    float posX;
    float posY;
    u16 pixelSizeX;
    u16 pixelSizeY;
    u32 color;
};

struct Box
{
    Vec2 pos;
    Vec2 size;
};

class VulkanFontDraw : public VulkanApp
{
public:
    virtual ~VulkanFontDraw() override;
    //bool initApp(const String &fontFilename);
    virtual bool init(const char *windowStr, i32 screenWidth, i32 screenHeight) override;

    bool initRun();
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

public:
    void updateCharacterImageData(u32 characterIndex);
    void updateDrawAreaData();
    Vec2 getDrawAreaStartPos() const;

    Image renderColorImage;

    Buffer quadBuffer[VulkanGlobal::FramesInFlight];
    Buffer indexDataBuffer;

    Pipeline graphicsPipeline;

    String fontFilename;

    u8 buffData[12] = {};
    i32 chosenLetter = 'a';

    // index 0 = selected box, 1..letters * lettersize = letter pixels, after that drawing area
    PodVector<GPUVertexData> vertData;
    PodVector<u8> characterData;

    PodVector<Box> charactersInScreen;

    static constexpr VkClearValue colorClear = { .color{ 0.0f, 0.5f, 1.0f, 1.0f } };
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanFontDraw::~VulkanFontDraw()
{
    destroyPipeline(graphicsPipeline);

    destroyImage(renderColorImage);
    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        destroyBuffer(quadBuffer[i]);
    destroyBuffer(indexDataBuffer);

}

bool VulkanFontDraw::init(const char *windowStr, i32 screenWidth, i32 screenHeight)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight))
        return false;
    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
    {
        quadBuffer[i] = createBuffer(QuadBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Quad buffer");
    }
    indexDataBuffer = createBuffer(32 * 1024 * 1024,
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
        offset = uploadToScratchbuffer(( void * ) indices.data(), size_t(sizeof(indices[ 0 ]) * indices.size()), offset);
        uploadScratchBufferToGpuBuffer(indexDataBuffer, offset);
    }

    {
        Pipeline& pipeline = graphicsPipeline;
        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);


        pipeline.renderPass = createRenderPass(
            { RenderTarget{ .format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR } },
            {});
        ASSERT(pipeline.renderPass);
        if(!pipeline.renderPass)
            return false;

        VkPipelineColorBlendAttachmentState rgbaAtt{ .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        if (!createGraphicsPipeline(
            getShader(ShaderType::ColoredQuadVert), getShader(ShaderType::ColoredQuadFrag),
            { { rgbaAtt } }, {}, pipeline, "Font draw render"))
        {
            printf("Failed to create pipeline\n");
            return false;
        }

        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),
                DescriptorInfo(quadBuffer[i]),
            };
        }
        if (!updateBindDescriptorSet(pipeline))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }

    return resized() && initRun();
}


bool VulkanFontDraw::initRun()
{
    if (!loadBytes(fontFilename.getStr(), characterData.getBuffer()))
    {
        printf("Failed to load file: %s\n", fontFilename.getStr());
        return false;
    }
    charactersInScreen.resize(LetterCount);

    vertData.resize(LetterHeight * LetterWidth * (LetterCount + 1) + 1);


    // selected red box
    {
        float offX = (borderSizes + buttonSize) + windowWidth * 0.5f;
        float offY = (borderSizes + buttonSize) + windowHeight * 0.5f;

        GPUVertexData& vdata = vertData[0];
        vdata.color = getColor(1.0f, 0.0f, 0.0f, 1.0f);
        vdata.pixelSizeX = u16(smallButtonSize) * LetterWidth + 4;
        vdata.pixelSizeY = u16(smallButtonSize) * LetterHeight + 4;
        vdata.posX = offX;
        vdata.posY = offY;
    }

    // Draw area boxes
    {
        updateDrawAreaData();
    }
    for (i32 k = 0; k < LetterCount; ++k)
    {
        i32 x = k % LetterWidth;
        i32 y = k / LetterWidth;
        float smallOffX =
            LetterPanelOffsetX + float(x * LetterWidth) * smallButtonSize + x * 2;
        float smallOffY =
            LetterPanelOffsetY + float(y * LetterHeight) * smallButtonSize + y * 2;

        charactersInScreen[k].pos = Vec2(smallOffX, smallOffY);
        charactersInScreen[k].size = Vec2(LetterWidth * smallButtonSize, LetterHeight * smallButtonSize);

        updateCharacterImageData(k +  LetterStartIndex);
    }
    return true;
}


bool VulkanFontDraw::resized()
{
    destroyImage(renderColorImage);

    // create color and depth images
    if (!createRenderTargetImage(
        vulk->swapchain.width, vulk->swapchain.height, vulk->defaultColorFormat,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        "Main color target image", renderColorImage))
    {
        printf("Failed to create color image\n");
        return false;
    }

    ASSERT(createFramebuffer(graphicsPipeline, { renderColorImage }));
    fontSystem.setRenderTarget(renderColorImage);

    return true;
}

void VulkanFontDraw::updateCharacterImageData(u32 characterIndex)
{
    ASSERT(characterIndex >=  LetterStartIndex && characterIndex <  LetterEndIndex);
    characterIndex -=  LetterStartIndex;

    const Vec2 startOffset = charactersInScreen[characterIndex].pos;
    const Vec2 size = charactersInScreen[characterIndex].size / Vec2(LetterWidth, LetterHeight);

    u32 startIndex = characterIndex * LetterHeight;
    u8 *visibility = (u8 *)&characterData[startIndex];

    // first LetterWidth * LetterHeight is reserved for the drawing middle.
    GPUVertexData *vertDataPtr = &vertData[(characterIndex + 1) * LetterWidth * LetterHeight + 1];

    for (u32 j = 0; j < LetterHeight; ++j)
    {
        u8 rowVisibility = visibility[j];
        for (u32 i = 0; i < LetterWidth; ++i)
        {
            bool isVisible = ((rowVisibility >> i) & 1) == 1;

            vertDataPtr->color = isVisible ? ~0u : 0u;
            vertDataPtr->posX = u16(startOffset.x + i * smallButtonSize);
            vertDataPtr->posY = u16(startOffset.y + j * smallButtonSize);
            vertDataPtr->pixelSizeX = u16(size.x);
            vertDataPtr->pixelSizeY = u16(size.y);
            ++vertDataPtr;
        }

    }

}

void VulkanFontDraw::updateDrawAreaData()
{
    ASSERT(chosenLetter >=  LetterStartIndex && chosenLetter <  LetterEndIndex);
    u32 startIndex = (chosenLetter -  LetterStartIndex) * LetterHeight;
    u8 *visibility = (u8 *)&characterData[startIndex];

        // Draw area boxes
    Vec2 drawStart = getDrawAreaStartPos();
    for (i32 j = 0; j < LetterHeight; ++j)
    {
        u8 visibilityBits = visibility[j];
        for (i32 i = 0; i < LetterWidth; ++i)
        {
            Vec2 pos = drawStart + Vec2(i, j) * (borderSizes + buttonSize);
            GPUVertexData& vdata = vertData[i + size_t(j) * LetterWidth + 1];
            vdata.color = ((visibilityBits >> i) & 1) ? ~0u : 0u;
            vdata.pixelSizeX = vdata.pixelSizeY = buttonSize;
            vdata.posX = pos.x;
            vdata.posY = pos.y;
        }
    }
}

Vec2 VulkanFontDraw::getDrawAreaStartPos() const
{
    return Vec2(
        float(LetterWidth * -0.5f) * (borderSizes + buttonSize) + windowWidth * 0.5f,
        float(LetterHeight * -0.5f) * (borderSizes + buttonSize) + windowHeight * 0.5f);
}


void VulkanFontDraw::logicUpdate()
{
    VulkanApp::logicUpdate();

    MouseState mouseState = getMouseState();
    {
        if (isPressed(GLFW_KEY_LEFT))
            --chosenLetter;

        if (isPressed(GLFW_KEY_RIGHT))
            ++chosenLetter;

        if (isPressed(GLFW_KEY_DOWN))
            chosenLetter += 8;

        if (isPressed(GLFW_KEY_UP))
            chosenLetter -= 8;

        bool isControlDown = keyDowns[GLFW_KEY_LEFT_CONTROL].isDown || keyDowns[GLFW_KEY_RIGHT_CONTROL].isDown;

        for (i32 i = 0; i < bufferedPressesCount; ++i)
        {
            if (!isControlDown && bufferedPresses[i] >=  LetterStartIndex && bufferedPresses[i] <  LetterEndIndex)
            {
                chosenLetter = (int)bufferedPresses[i];
            }
        }

        if (chosenLetter <  LetterStartIndex)
            chosenLetter =  LetterStartIndex;
        if (chosenLetter >=  LetterEndIndex)
            chosenLetter =  LetterEndIndex - 1;


        if (keyDowns[GLFW_KEY_S].isDown && keyDowns[GLFW_KEY_S].pressCount > 0u && isControlDown)
            writeBytes(fontFilename.getStr(), characterData.getBuffer());

        if (keyDowns[GLFW_KEY_L].isDown && keyDowns[GLFW_KEY_L].pressCount > 0u && isControlDown)
        {
            loadBytes(fontFilename.getStr(), characterData.getBuffer());
            for(u32 i  = 0; i < LetterCount; ++i)
                updateCharacterImageData(i +  LetterStartIndex);
        }

        if (keyDowns[GLFW_KEY_C].isDown && keyDowns[GLFW_KEY_C].pressCount > 0u && isControlDown)
        {
            u32 ind = (chosenLetter -  LetterStartIndex) * LetterHeight;
            u8 *ptr = &characterData[ind];
            for (i32 i = 0; i < LetterHeight; ++i)
            {
                buffData[i] = ptr[i];
            }
        }

        if (keyDowns[GLFW_KEY_V].isDown && keyDowns[GLFW_KEY_V].pressCount > 0u && isControlDown)
        {
            u32 ind = (chosenLetter -  LetterStartIndex) * LetterHeight;
            u8 *ptr = &characterData[ind];
            for (i32 i = 0; i < LetterHeight; ++i)
            {
                ptr[i] = buffData[i];
            }
        }
        // check if we click any of the characters?
        if(mouseState.leftButtonDown)
        {
            for(u32 i = 0; i < charactersInScreen.size(); ++i)
            {
                const auto &box = charactersInScreen[i];
                if(mouseState.x >= box.pos.x && mouseState.x <= box.pos.x + box.size.x &&
                    mouseState.y >= box.pos.y && mouseState.y <= box.pos.y + box.size.y)
                {
                    chosenLetter =  LetterStartIndex + i;
                    break;
                }
            }
        }
        // Check if drawing or erasing font letter pixels
        if(mouseState.leftButtonDown || mouseState.rightButtonDown)
        {
            Vec2 drawBox = Vec2(mouseState.x, mouseState.y) - getDrawAreaStartPos();

            i32 foundI = drawBox.x / ((borderSizes + buttonSize));
            i32 foundJ = drawBox.y / ((borderSizes + buttonSize));
            // -0.1 / 1 = 0... so cannot check if foundI >= 0
            if(drawBox.x >= 0.0f && foundI < LetterWidth
                && drawBox.y >= 0.0f && foundJ < LetterHeight)
            {
                u8 *visiblePtr = &characterData[(chosenLetter -  LetterStartIndex) * LetterHeight];
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
        updateCharacterImageData(chosenLetter);
        updateDrawAreaData();

        u32 xOff = ((chosenLetter -  LetterStartIndex) % LetterWidth);
        u32 yOff = ((chosenLetter -  LetterStartIndex) / LetterWidth);

        vertData[0].posX = LetterPanelOffsetX + (xOff * LetterWidth ) * smallButtonSize + xOff * 2 - 2;
        vertData[0].posY = LetterPanelOffsetY + (yOff * LetterHeight) * smallButtonSize + yOff * 2 - 2;
    }

    ASSERT(vertData.size() * sizeof(GPUVertexData) < QuadBufferSize);
}

void VulkanFontDraw::renderUpdate()
{
    VulkanApp::renderUpdate();

    addToCopylist(sliceFromPodVectorBytes(vertData), quadBuffer[vulk->frameIndex]);
}

void VulkanFontDraw::renderDraw()
{
    prepareToGraphicsSampleWrite(renderColorImage);

    // Drawingg

    {

        beginRenderPass(graphicsPipeline, { colorClear });
        // draw calls here
        // Render
        {
            bindGraphicsPipelineWithDecriptors(graphicsPipeline, vulk->frameIndex);
            vkCmdBindIndexBuffer(vulk->commandBuffer, indexDataBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulk->commandBuffer, u32(vertData.size() * 6), 1, 0, 0, 0);

        }
        vkCmdEndRenderPass(vulk->commandBuffer);
        //vkCmdEndRendering(vulk->commandBuffer);
    }

    writeStamp();

    present(renderColorImage);

}





i32 main(i32 argCount, char **argv)
{
    initMemory();
    {
        String filename;
        if (argCount < 2)
        {
            filename = "assets/font/new_font.dat";
        }
        else
        {
            filename = argv[ 1 ];
        }
        VulkanFontDraw app;
        app.fontFilename = filename;
        if(app.init("Vulkan, draw font", SCREEN_WIDTH, SCREEN_HEIGHT))
        {
            app.run();
        }
    }
    deinitMemory();
    return 0;
}