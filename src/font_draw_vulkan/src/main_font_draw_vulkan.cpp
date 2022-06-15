
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <core/file.h>
#include <core/general.h>
#include <core/mytypes.h>
#include <core/timer.h>
#include <core/vulkan_app.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string.h>

static constexpr int SCREEN_WIDTH  = 640;
static constexpr int SCREEN_HEIGHT = 540;

static constexpr VkDeviceSize QuadBufferSize = 8 * 1024 * 1024u;

static constexpr float buttonSize = 20.0f;
static constexpr float smallButtonSize = 2.0f;
static constexpr float borderSizes = 2.0f;

struct GPUVertexData
{
    float posX;
    float posY;
    uint16_t pixelSizeX;
    uint16_t pixelSizeY;
    uint32_t color;
};

class VulkanFontDraw : public VulkanApp
{
public:
    virtual ~VulkanFontDraw() override;
    //bool initApp(const std::string &fontFilename);
    virtual bool init(const char *windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters &params) override;

    bool initRun();
    virtual void logicUpdate() override;
    virtual void renderUpdate() override;
    virtual void renderDraw() override;
    virtual bool resized() override;

public:
    Image renderColorImage;

    Buffer quadBuffer;
    Buffer indexDataBuffer;

    Pipeline graphicsPipeline;

    std::string fontFilename;

    char buffData[12] = {};
    int32_t chosenLetter = 'a';

    PodVector<GPUVertexData> vertData;
    PodVector<char> characterData;
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

    destroyBuffer(quadBuffer);
    destroyBuffer(indexDataBuffer);

}

bool VulkanFontDraw::init(const char *windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters &params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;

    quadBuffer = createBuffer(QuadBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Quad buffer");

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

        if (!createGraphicsPipeline(
            getShader(ShaderType::ColoredQuadVert), getShader(ShaderType::ColoredQuadFrag),
            { RenderTarget {.format = vulk->defaultColorFormat } }, {  }, pipeline, "Font draw render", false))
        {
            printf("Failed to create pipeline\n");
            return false;
        }


        pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
            {
                DescriptorInfo(vulk->renderFrameBufferHandle),
                DescriptorInfo(quadBuffer),
            });

        if (!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }
    
    return resized() && initRun();
}


bool VulkanFontDraw::initRun()
{
    const uint32_t charCount = 128 - 32;

    if (!loadBytes(fontFilename, characterData))
    {
        printf("Failed to load file: %s\n", fontFilename.c_str());
        return false;
    }


    vertData.resize(12 * 8 * (charCount + 1) + 1);



    {
        float offX = (borderSizes + buttonSize) + windowWidth * 0.5f;
        float offY = (borderSizes + buttonSize) + windowHeight * 0.5f;

        GPUVertexData& vdata = vertData[0];
        vdata.color = getColor(1.0f, 0.0f, 0.0f, 1.0f);
        vdata.pixelSizeX = uint16_t(smallButtonSize) * 8 + 4;
        vdata.pixelSizeY = uint16_t(smallButtonSize) * 12 + 4;
        vdata.posX = offX;
        vdata.posY = offY;
    }

    for (int j = 0; j < 12; ++j)
    {
        for (int i = 0; i < 8; ++i)
        {
            float offX = float((i - 4) * (borderSizes + buttonSize)) + windowWidth * 0.5f;// -buttonSize * 0.5f;
            float offY = float((j - 6) * (borderSizes + buttonSize)) + windowHeight * 0.5f;// -buttonSize * 0.5f;

            GPUVertexData& vdata = vertData[i + size_t(j) * 8 + 1];
            vdata.color = 0;
            vdata.pixelSizeX = vdata.pixelSizeY = buttonSize;
            vdata.posX = offX;
            vdata.posY = offY;
        }
    }

    for (int k = 0; k < charCount; ++k)
    {
        int x = k % 8;
        int y = k / 8;
        for (int j = 0; j < 12; ++j)
        {
            for (int i = 0; i < 8; ++i)
            {
                GPUVertexData& vdata = vertData[i + size_t(j) * 8 + (size_t(k) + 1) * 8 * 12 + 1];

                float smallOffX = float(i * (smallButtonSize)) + 10.0f + float(x * 8) * smallButtonSize + x * 2;
                float smallOffY = float(j * (smallButtonSize)) + 10.0f + float(y * 12) * smallButtonSize + y * 2;

                uint32_t indx = k * 12 + j;
                bool isVisible = ((characterData[indx] >> i) & 1) == 1;

                vdata.color = isVisible ? ~0u : 0u;
                vdata.pixelSizeX = vdata.pixelSizeY = smallButtonSize;
                vdata.posX = smallOffX;
                vdata.posY = smallOffY;

            }
        }
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
            if (!isControlDown && bufferedPresses[i] >= 32 && bufferedPresses[i] < 128)
            {
                chosenLetter = (int)bufferedPresses[i];
            }
        }

        if (keyDowns[GLFW_KEY_S].isDown && keyDowns[GLFW_KEY_S].pressCount > 0u && isControlDown)
            writeBytes(fontFilename, sliceFromPodVectorBytes(characterData));

        if (keyDowns[GLFW_KEY_L].isDown && keyDowns[GLFW_KEY_L].pressCount > 0u && isControlDown)
            loadBytes(fontFilename, characterData);

        if (keyDowns[GLFW_KEY_C].isDown && keyDowns[GLFW_KEY_C].pressCount > 0u && isControlDown)
        {
            for (int i = 0; i < 12; ++i)
            {
                uint32_t ind = (chosenLetter - 32) * 12 + i;
                buffData[i] = characterData[ind];
            }
        }

        if (keyDowns[GLFW_KEY_V].isDown && keyDowns[GLFW_KEY_V].pressCount > 0u && isControlDown)
        {
            for (int i = 0; i < 12; ++i)
            {
                uint32_t ind = (chosenLetter - 32) * 12 + i;
                characterData[ind] = char(buffData[i]);
            }
        }

        if (chosenLetter < 32)
            chosenLetter = 32;
        if (chosenLetter > 127)
            chosenLetter = 127;

        for (int j = 0; j < 12; ++j)
        {

            for (int i = 0; i < 8; ++i)
            {
                float offX = float((i - 4) * (borderSizes + buttonSize)) + windowWidth * 0.5f;
                float offY = float((j - 6) * (borderSizes + buttonSize)) + windowHeight * 0.5f;

                bool insideRect = mouseState.x > offX - (borderSizes + buttonSize) * 0.5f &&
                    mouseState.x < offX + (borderSizes + buttonSize) * 0.5f &&
                    mouseState.y > offY - (borderSizes + buttonSize) * 0.5f &&
                    mouseState.y < offY + (borderSizes + buttonSize) * 0.5f;

                offX -= 0.5f * buttonSize;
                offY -= 0.5f * buttonSize;

                uint32_t indx = (chosenLetter - 32) * 12 + j;

                if (mouseState.leftButtonDown && insideRect)
                    characterData[indx] |= (1 << i);
                else if (mouseState.rightButtonDown && insideRect)
                    characterData[indx] &= ~(char(1 << i));

                bool isVisible = ((characterData[indx] >> i) & 1) == 1;

                vertData[i + size_t(j) * 8 + 1].color = isVisible ? ~0u : 0u;
                vertData[i + size_t(j) * 8 + 1].posX = uint16_t(offX);
                vertData[i + size_t(j) * 8 + 1].posY = uint16_t(offY);
                vertData[(size_t(indx) + 12) * 8 + i + 1].color = isVisible ? ~0u : 0u;

            }

        }
        uint32_t xOff = (chosenLetter - 32) % 8;
        uint32_t yOff = (chosenLetter - 32) / 8;

        vertData[0].posX = 10.0f + (xOff * 8) * smallButtonSize + xOff * 2 - 2;
        vertData[0].posY = 10.0f + (yOff * 12) * smallButtonSize + yOff * 2 - 2;
    }

    ASSERT(vertData.size() * sizeof(GPUVertexData) < QuadBufferSize);
}

void VulkanFontDraw::renderUpdate()
{
    VulkanApp::renderUpdate();

    addToCopylist(sliceFromPodVectorBytes(vertData), quadBuffer.buffer, 0);
}

void VulkanFontDraw::renderDraw()
{
    prepareToGraphicsSampleWrite(renderColorImage);

    // Drawingg

    {
        static constexpr VkClearValue colorClear = { .color{0.0f, 0.5f, 1.0f, 1.0f} };
        beginRenderPass(graphicsPipeline, { colorClear });
        // draw calls here
        // Render
        {
            bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
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
    std::string filename;
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

    return 0;
}