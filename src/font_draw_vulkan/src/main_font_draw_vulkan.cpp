
#include "core/general.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

#include "core/vulkan_app.h"

#include "core/timer.h"
#include "core/mytypes.h"

#include "myvulkan/myvulkan.h"

#include "math/general_math.h"
#include "math/matrix.h"
#include "math/plane.h"
#include "math/quaternion.h"
#include "math/vector3.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>
#include <string.h>


static constexpr int SCREEN_WIDTH  = 640;
static constexpr int SCREEN_HEIGHT = 540;

static constexpr VkDeviceSize QuadBufferSize = 8 * 1024 * 1024u;

struct GPUVertexData
{
    float posX;
    float posY;
    uint16_t pixelSizeX;
    uint16_t pixelSizeY;
    uint32_t color;
};



bool saveFontData(const std::string &filename, const PodVector<char> &data)
{
    //
    if(std::filesystem::exists(filename))
    {
        std::filesystem::path p(filename);


        std::ofstream f(p, std::ios::out | std::ios::binary);


        f.write(data.data(), data.size());

        printf("filesize: %u\n", uint32_t(data.size()));
        return true;
    }
    return false;
}



enum TIME_POINTS
{
    START_POINT,
    DRAW_FINISHED,

    NUM_TIME_POINTS
};


class VulkanFontDraw : public VulkanApp
{
public:
    virtual ~VulkanFontDraw() override;
    //bool initApp(const std::string &fontFilename);
    virtual bool init(const char *windowStr, int screenWidth, int screenHeight, 
        const VulkanInitializationParameters &params) override;
    virtual void run() override;

public:
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;

    Buffer quadBuffer;
    Buffer indexDataBuffer;

    DescriptorSet descriptorSet;
    PipelineWithDescriptors graphicsPipeline;

    std::string fontFilename;
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanFontDraw::~VulkanFontDraw()
{

    destroyDescriptor(graphicsPipeline.descriptor);
    destroyPipeline(graphicsPipeline);

    destroyBuffer(quadBuffer);
    destroyBuffer(indexDataBuffer);

    destroyShaderModule(fragShaderModule);
    destroyShaderModule(vertShaderModule);

}

bool VulkanFontDraw::init(const char *windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters &params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;

    glfwSetWindowUserPointer(window, this);

    vertShaderModule = loadShader("assets/shader/vulkan_new/coloredquad.vert.spv");
    ASSERT(vertShaderModule);

    fragShaderModule = loadShader("assets/shader/vulkan_new/coloredquad.frag.spv");
    ASSERT(fragShaderModule);



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
        std::vector<uint32_t> indices;
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

    PipelineWithDescriptors &pipeline = graphicsPipeline;

    pipeline.descriptorSetLayouts = PodVector<DescriptorSetLayout>({
        DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u },
        DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u },
        });


    if (!createPipelineLayout(pipeline, VK_SHADER_STAGE_ALL_GRAPHICS))
    {
        printf("Failed to create pipelinelayout!\n");
        return false;
    }

    pipeline.pipeline = createGraphicsPipeline(vulk.renderPass,
        vertShaderModule, fragShaderModule,
        pipeline.pipelineLayout, false);


    pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
        {
            DescriptorInfo(vulk.renderFrameBufferHandle),
            DescriptorInfo(quadBuffer.buffer, 0u, QuadBufferSize),
        });

    pipeline.descriptor = createDescriptor(pipeline.descriptorSetLayouts, pipeline.descriptorSetLayout);
    if (!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
    {
        printf("Failed to set descriptor binds!\n");
        return false;
    }

    return true;
}


void VulkanFontDraw::run()
{
    const uint32_t charCount = 128 - 32;
    PodVector<char> data;
    if (!loadBytes(fontFilename, data))
    {
        printf("Failed to load file: %s\n", fontFilename.c_str());
        return;
    }

    int32_t chosenLetter = 'a';

    std::vector<GPUVertexData> vertData;
    vertData.resize(12 * 8 * ( charCount + 1 ) + 1);


    static constexpr float buttonSize = 20.0f;
    static constexpr float smallButtonSize = 2.0f;
    static constexpr float borderSizes = 2.0f;

    {
        float offX = ( borderSizes + buttonSize ) + windowWidth * 0.5f;
        float offY = ( borderSizes + buttonSize ) + windowHeight * 0.5f;

        GPUVertexData &vdata = vertData[ 0 ];
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
            float offX = float(( i - 4 ) * ( borderSizes + buttonSize )) + windowWidth * 0.5f;// -buttonSize * 0.5f;
            float offY = float(( j - 6 ) * ( borderSizes + buttonSize )) + windowHeight * 0.5f;// -buttonSize * 0.5f;

            GPUVertexData &vdata = vertData[ i + size_t(j) * 8 + 1 ];
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
                GPUVertexData &vdata = vertData[ i + size_t(j) * 8 + ( size_t(k) + 1 ) * 8 * 12 + 1 ];

                float smallOffX = float(i * ( smallButtonSize )) + 10.0f + float(x * 8) * smallButtonSize + x * 2;
                float smallOffY = float(j * ( smallButtonSize )) + 10.0f + float(y * 12) * smallButtonSize + y * 2;

                uint32_t indx = k * 12 + j;
                bool isVisible = ( ( data[ indx ] >> i ) & 1 ) == 1;

                vdata.color = isVisible ? ~0u : 0u;
                vdata.pixelSizeX = vdata.pixelSizeY = smallButtonSize;
                vdata.posX = smallOffX;
                vdata.posY = smallOffY;

            }
        }
    }

    char buffData[ 12 ] = {};



    ////////////////////////
    //
    // MAIN LOOP START
    // UPDATING ENGINE STATE
    //
    ////////////////////////

    double previousFrameTime = glfwGetTime();
    uint32_t framesSinceLastDelta = 0u;
    double deltaTime = 0.0;

    uint32_t gpuframeCount = 0u;
    double gpuTime = 0.0;
    double cpuTimeStamp = glfwGetTime();


    while (!glfwWindowShouldClose(window))
    {
        if (++framesSinceLastDelta > 10)
        {
            double newTime = glfwGetTime();
            deltaTime = ( newTime - previousFrameTime ) / framesSinceLastDelta;
            previousFrameTime = newTime;
            framesSinceLastDelta = 0u;
        }

        glfwPollEvents();
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



            bool isControlDown = keyDowns[ GLFW_KEY_LEFT_CONTROL ].isDown || keyDowns[ GLFW_KEY_RIGHT_CONTROL ].isDown;

            for (int i = 0; i < bufferedPressesCount; ++i)
            {
                if (!isControlDown && bufferedPresses[ i ] >= 32 && bufferedPresses[ i ] < 128)
                {
                    chosenLetter = ( int ) bufferedPresses[ i ];
                }
            }

            if (keyDowns[ GLFW_KEY_S ].isDown && keyDowns[ GLFW_KEY_S ].pressCount > 0u && isControlDown)
                saveFontData(fontFilename, data);

            if (keyDowns[ GLFW_KEY_L ].isDown && keyDowns[ GLFW_KEY_L ].pressCount > 0u && isControlDown)
                loadBytes(fontFilename, data);

            if (keyDowns[ GLFW_KEY_C ].isDown && keyDowns[ GLFW_KEY_C ].pressCount > 0u && isControlDown)
            {
                for (int i = 0; i < 12; ++i)
                {
                    uint32_t ind = ( chosenLetter - 32 ) * 12 + i;
                    buffData[ i ] = data[ ind ];
                }
            }

            if (keyDowns[ GLFW_KEY_V ].isDown && keyDowns[ GLFW_KEY_V ].pressCount > 0u && isControlDown)
            {
                for (int i = 0; i < 12; ++i)
                {
                    uint32_t ind = ( chosenLetter - 32 ) * 12 + i;
                    data[ ind ] = char(buffData[ i ]);
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
                    float offX = float(( i - 4 ) * ( borderSizes + buttonSize )) + windowWidth * 0.5f;
                    float offY = float(( j - 6 ) * ( borderSizes + buttonSize )) + windowHeight * 0.5f;

                    bool insideRect = mouseState.x > offX - ( borderSizes + buttonSize ) * 0.5f &&
                        mouseState.x < offX + ( borderSizes + buttonSize ) * 0.5f &&
                        mouseState.y > offY - ( borderSizes + buttonSize ) * 0.5f &&
                        mouseState.y < offY + ( borderSizes + buttonSize ) * 0.5f;

                    offX -= 0.5f * buttonSize;
                    offY -= 0.5f * buttonSize;

                    uint32_t indx = ( chosenLetter - 32 ) * 12 + j;

                    if (mouseState.leftButtonDown && insideRect)
                        data[ indx ] |= ( 1 << i );
                    else if (mouseState.rightButtonDown && insideRect)
                        data[ indx ] &= ~( char(1 << i) );

                    bool isVisible = ( ( data[ indx ] >> i ) & 1 ) == 1;

                    vertData[ i + size_t(j) * 8 + 1 ].color = isVisible ? ~0u : 0u;
                    vertData[ i + size_t(j) * 8 + 1 ].posX = uint16_t(offX);
                    vertData[ i + size_t(j) * 8 + 1 ].posY = uint16_t(offY);
                    vertData[ ( size_t(indx) + 12 ) * 8 + i + 1 ].color = isVisible ? ~0u : 0u;

                }

            }
            uint32_t xOff = ( chosenLetter - 32 ) % 8;
            uint32_t yOff = ( chosenLetter - 32 ) / 8;

            vertData[ 0 ].posX = 10.0f + ( xOff * 8 ) * smallButtonSize + xOff * 2 - 2;
            vertData[ 0 ].posY = 10.0f + ( yOff * 12 ) * smallButtonSize + yOff * 2 - 2;
        }

        ASSERT(vertData.size() * sizeof(GPUVertexData) < QuadBufferSize);


        ////////////////////////
        //
        // RENDER PASSES START
        // WRITING VALUES INTO
        // "CONSTANT BUFFEERS"
        //
        ////////////////////////

        if (!startRender(window))
            continue;

        beginSingleTimeCommands();
        vkCmdResetQueryPool(vulk.commandBuffer, vulk.queryPool, 0, QUERY_COUNT);
        vkCmdWriteTimestamp(vulk.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vulk.queryPool, TIME_POINTS::START_POINT);


        {
            updateRenderFrameBuffer();

            uint32_t offset = vulk.scratchBufferOffset;
            // use scratch buffer to unifrom buffer transfer
            uint32_t vertDataSize = uint32_t(vertData.size() * sizeof(GPUVertexData));
            memcpy(( void * ) ( ( char * ) vulk.scratchBuffer.data + offset), vertData.data(), vertDataSize);

            {
                VkBufferCopy region = { offset, 0, VkDeviceSize(vertDataSize) };
                vkCmdCopyBuffer(vulk.commandBuffer, vulk.scratchBuffer.buffer, quadBuffer.buffer, 1, &region);
            }

            VkBufferMemoryBarrier bar[]
            {
                bufferBarrier(quadBuffer.buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, vertDataSize),
            };

            vkCmdPipelineBarrier(vulk.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                    VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZES(bar), bar, 0, nullptr);
            vulk.scratchBufferOffset += vertDataSize;

        }



        ////////////////////////
        //
        // MAIN RENDER
        //
        ////////////////////////
        {
            VkImageMemoryBarrier imageBarriers[] =
            {
                imageBarrier(vulk.mainColorRenderTarget,
                            0, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),

                imageBarrier(vulk.mainDepthRenderTarget,
                            0, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                            VK_IMAGE_ASPECT_DEPTH_BIT),
            };

            vkCmdPipelineBarrier(vulk.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                 VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZES(imageBarriers), imageBarriers);
        }

        // Drawingg
        {
            VkClearValue clearValues[ 2 ] = {};
            clearValues[ 0 ].color = VkClearColorValue{ {0.0f, 0.5f, 1.0f, 1.0f } };
            clearValues[ 1 ].depthStencil = { 0.0f, 0 };

            VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
            passBeginInfo.renderPass = vulk.renderPass;
            passBeginInfo.framebuffer = vulk.targetFB;
            passBeginInfo.renderArea.extent.width = vulk.swapchain.width;
            passBeginInfo.renderArea.extent.height = vulk.swapchain.height;
            passBeginInfo.clearValueCount = ARRAYSIZES(clearValues);
            passBeginInfo.pClearValues = clearValues;

            vkCmdBeginRenderPass(vulk.commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewPort = { 0.0f, float(vulk.swapchain.height), float(vulk.swapchain.width), -float(vulk.swapchain.height), 0.0f, 1.0f };
            VkRect2D scissors = { { 0, 0 }, { uint32_t(vulk.swapchain.width), uint32_t(vulk.swapchain.height) } };

            insertDebugRegion("Render", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
            vkCmdSetViewport(vulk.commandBuffer, 0, 1, &viewPort);
            vkCmdSetScissor(vulk.commandBuffer, 0, 1, &scissors);

            // draw calls here
            // Render
            {
                bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
                vkCmdBindIndexBuffer(vulk.commandBuffer, indexDataBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(vulk.commandBuffer, uint32_t(vertData.size() * 6), 1, 0, 0, 0);

            }
            vkCmdEndRenderPass(vulk.commandBuffer);
        }

        vkCmdWriteTimestamp(vulk.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, vulk.queryPool, TIME_POINTS::DRAW_FINISHED);

        present(window);

        ////////////////////////
        //
        // END PASS, COLLECT TIMINGS
        //
        ////////////////////////


        uint64_t queryResults[ TIME_POINTS::NUM_TIME_POINTS ];
        vkGetQueryPoolResults(vulk.device, vulk.queryPool, 
            0, ARRAYSIZES(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[ 0 ]), VK_QUERY_RESULT_64_BIT);


        struct TimeValues
        {
            double timeDuration[ TIME_POINTS::NUM_TIME_POINTS ];
        };

        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(vulk.physicalDevice, &props);

        static TimeValues timeValues = {};
        for (uint32_t i = TIME_POINTS::NUM_TIME_POINTS - 1; i > 0; --i)
            timeValues.timeDuration[ i ] += ( double(queryResults[ i ]) - double(queryResults[ i - 1 ]) ) * props.limits.timestampPeriod * 1.0e-9f;

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
                   ( float ) ( timeValues.timeDuration[ DRAW_FINISHED ] * d ),
                   e / gpuTime, e / cpuTime);
            gpuframeCount = 0u;

            for (uint32_t i = 0; i < TIME_POINTS::NUM_TIME_POINTS; ++i)
                timeValues.timeDuration[ i ] = 0.0;

            gpuTime = 0.0;
        }


        char str[ 100 ];
        char renderLetter = chosenLetter != 127 ? char(chosenLetter) : ' ';
        float fps = dt > 0.0 ? float(1.0 / dt) : 0.0f;
        sprintf(str, "%2.2fms, fps: %4.2f, mx: %i, my: %i, ml: %i, mr: %i, mb: %i, Letter: %c",
                float(dt * 1000.0), fps,
                mouseState.x, mouseState.y, mouseState.leftButtonDown, mouseState.rightButtonDown, mouseState.middleButtonDown,
                renderLetter);
        setTitle(str);


        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    }

    VK_CHECK(vkDeviceWaitIdle(vulk.device));

}





int main(int argCount, char **argv)
{
    std::vector<char> data;
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