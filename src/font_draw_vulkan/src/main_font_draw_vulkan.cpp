
#include <container/podvector.h>
#include <container/podvectortypedefine.h>
#include <container/string.h>
#include <container/vector.h>

#include <core/file.h>
#include <core/glfw_keys.h>
#include <core/general.h>
#include <core/mytypes.h>
#include <core/timer.h>
#include <core/vulkan_app.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>
#include <math/vector3_inline_functions.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

static constexpr int SCREEN_WIDTH  = 640;
static constexpr int SCREEN_HEIGHT = 540;

static constexpr VkDeviceSize QuadBufferSize = 8 * 1024 * 1024u;

static constexpr float buttonSize = 20.0f;
static constexpr float smallButtonSize = 2.0f;
static constexpr float borderSizes = 2.0f;

static constexpr uint32_t  LetterStartIndex = 32;
static constexpr uint32_t  LetterEndIndex = 128;
static constexpr uint32_t LetterCount =  LetterEndIndex -  LetterStartIndex;

static constexpr uint32_t LetterWidth = 8;
static constexpr uint32_t LetterHeight = 12;

static constexpr float LetterPanelOffsetX = 10.0f;
static constexpr float LetterPanelOffsetY = 10.0f;

struct GPUVertexData
{
    float posX;
    float posY;
    uint16_t pixelSizeX;
    uint16_t pixelSizeY;
    uint32_t color;
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
    virtual bool init(const char *windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters &params) override;

    bool initRun();
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

public:
    void updateCharacterImageData(uint32_t characterIndex);
    void updateDrawAreaData();
    Vec2 getDrawAreaStartPos() const;

    Image renderColorImage;

    Buffer quadBuffer[VulkanGlobal::FramesInFlight];
    Buffer indexDataBuffer;

    Pipeline graphicsPipeline;

    String fontFilename;

    uint8_t buffData[12] = {};
    int32_t chosenLetter = 'a';

    // index 0 = selected box, 1..letters * lettersize = letter pixels, after that drawing area
    PodVector<GPUVertexData> vertData;
    PodVector<uint8_t> characterData;

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
    for(uint32_t i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        destroyBuffer(quadBuffer[i]);
    destroyBuffer(indexDataBuffer);

}

bool VulkanFontDraw::init(const char *windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters &params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;
    for(uint32_t i = 0; i < VulkanGlobal::FramesInFlight; ++i)
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
    //setObjectTag(device, (uint64_t)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);


    {
        uint32_t offset = 0;
        PodVector<uint32_t> indices;
        indices.resize(6 * 10240);
        for (int i = 0; i < 10240; ++i)
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

        for(uint32_t i = 0; i < VulkanGlobal::FramesInFlight; ++i)
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
        vdata.pixelSizeX = uint16_t(smallButtonSize) * LetterWidth + 4;
        vdata.pixelSizeY = uint16_t(smallButtonSize) * LetterHeight + 4;
        vdata.posX = offX;
        vdata.posY = offY;
    }

    // Draw area boxes
    {
        updateDrawAreaData();
    }
    for (int k = 0; k < LetterCount; ++k)
    {
        int x = k % LetterWidth;
        int y = k / LetterWidth;
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

void VulkanFontDraw::updateCharacterImageData(uint32_t characterIndex)
{
    ASSERT(characterIndex >=  LetterStartIndex && characterIndex <  LetterEndIndex);
    characterIndex -=  LetterStartIndex;

    const Vec2 startOffset = charactersInScreen[characterIndex].pos;
    const Vec2 size = charactersInScreen[characterIndex].size / Vec2(LetterWidth, LetterHeight);

    uint32_t startIndex = characterIndex * LetterHeight;
    uint8_t *visibility = (uint8_t *)&characterData[startIndex];

    // first LetterWidth * LetterHeight is reserved for the drawing middle.
    GPUVertexData *vertDataPtr = &vertData[(characterIndex + 1) * LetterWidth * LetterHeight + 1];

    for (uint32_t j = 0; j < LetterHeight; ++j)
    {
        uint8_t rowVisibility = visibility[j];
        for (uint32_t i = 0; i < LetterWidth; ++i)
        {
            bool isVisible = ((rowVisibility >> i) & 1) == 1;

            vertDataPtr->color = isVisible ? ~0u : 0u;
            vertDataPtr->posX = uint16_t(startOffset.x + i * smallButtonSize);
            vertDataPtr->posY = uint16_t(startOffset.y + j * smallButtonSize);
            vertDataPtr->pixelSizeX = uint16_t(size.x);
            vertDataPtr->pixelSizeY = uint16_t(size.y);
            ++vertDataPtr;
        }

    }

}

void VulkanFontDraw::updateDrawAreaData()
{
    ASSERT(chosenLetter >=  LetterStartIndex && chosenLetter <  LetterEndIndex);
    uint32_t startIndex = (chosenLetter -  LetterStartIndex) * LetterHeight;
    uint8_t *visibility = (uint8_t *)&characterData[startIndex];

        // Draw area boxes
    Vec2 drawStart = getDrawAreaStartPos();
    for (int j = 0; j < LetterHeight; ++j)
    {
        uint8_t visibilityBits = visibility[j];
        for (int i = 0; i < LetterWidth; ++i)
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

        for (int i = 0; i < bufferedPressesCount; ++i)
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
            for(uint32_t i  = 0; i < LetterCount; ++i)
                updateCharacterImageData(i +  LetterStartIndex);
        }

        if (keyDowns[GLFW_KEY_C].isDown && keyDowns[GLFW_KEY_C].pressCount > 0u && isControlDown)
        {
            uint32_t ind = (chosenLetter -  LetterStartIndex) * LetterHeight;
            uint8_t *ptr = &characterData[ind];
            for (int i = 0; i < LetterHeight; ++i)
            {
                buffData[i] = ptr[i];
            }
        }

        if (keyDowns[GLFW_KEY_V].isDown && keyDowns[GLFW_KEY_V].pressCount > 0u && isControlDown)
        {
            uint32_t ind = (chosenLetter -  LetterStartIndex) * LetterHeight;
            uint8_t *ptr = &characterData[ind];
            for (int i = 0; i < LetterHeight; ++i)
            {
                ptr[i] = buffData[i];
            }
        }
        // check if we click any of the characters?
        if(mouseState.leftButtonDown)
        {
            for(uint32_t i = 0; i < charactersInScreen.size(); ++i)
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

            int foundI = drawBox.x / ((borderSizes + buttonSize));
            int foundJ = drawBox.y / ((borderSizes + buttonSize));
            // -0.1 / 1 = 0... so cannot check if foundI >= 0
            if(drawBox.x >= 0.0f && foundI < LetterWidth
                && drawBox.y >= 0.0f && foundJ < LetterHeight)
            {
                uint8_t *visiblePtr = &characterData[(chosenLetter -  LetterStartIndex) * LetterHeight];
                uint8_t &visibleRow = visiblePtr[foundJ];
                uint8_t bit = (1 << foundI);
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

        uint32_t xOff = ((chosenLetter -  LetterStartIndex) % LetterWidth);
        uint32_t yOff = ((chosenLetter -  LetterStartIndex) / LetterWidth);

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
            vkCmdDrawIndexed(vulk->commandBuffer, uint32_t(vertData.size() * 6), 1, 0, 0, 0);

        }
        vkCmdEndRenderPass(vulk->commandBuffer);
        //vkCmdEndRendering(vulk->commandBuffer);
    }

    writeStamp();

    present(renderColorImage);

}





int main(int argCount, char **argv)
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
        if(app.init("Vulkan, draw font", SCREEN_WIDTH, SCREEN_HEIGHT,
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
    }
    deinitMemory();
    return 0;
}