
#include "core/general.h"
#include "core/json.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

#include "core/vulkan_app.h"


#include "core/camera.h"
#include "core/timer.h"
#include "core/mytypes.h"
#include "core/transform.h"

#include <container/arraysliceview.h>

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
#include <string.h>

static constexpr int SCREEN_WIDTH = 640;
static constexpr int SCREEN_HEIGHT = 540;



enum TIME_POINTS
{
    START_POINT,
    DRAW_FINISHED,

    NUM_TIME_POINTS
};


// Probably not good in long run?
enum PipelineWithDescriptorsIndexes
{
    PIPELINE_GRAPHICS_PIPELINE,
    NUM_PIPELINE
};


class VulkanDrawStuff : public VulkanApp
{
public:
    VulkanDrawStuff() { }
    virtual ~VulkanDrawStuff() override;
    //bool initApp(const std::string &fontFilename);
    virtual bool init(const char* windowStr, int screenWidth, int screenHeight,
        const VulkanInitializationParameters& params) override;
    virtual void update() override;

    bool createPipelines();

public:
    Camera camera;

    UniformBufferHandle uniformDataHandle;
    VkShaderModule vertShaderModule = { };
    VkShaderModule fragShaderModule = { };
    Buffer quadBuffer;
    Buffer indexDataBuffer;

    DescriptorSet descriptorSet;
    PipelineWithDescriptors graphicsPipeline;

    std::string fontFilename;

    uint32_t indicesCount = 0;
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanDrawStuff::~VulkanDrawStuff()
{
    destroyDescriptor(graphicsPipeline.descriptor);
    destroyPipeline(graphicsPipeline);

    destroyBuffer(quadBuffer);
    destroyBuffer(indexDataBuffer);

    vkDestroyShaderModule(vulk.device, vertShaderModule, nullptr);
    vkDestroyShaderModule(vulk.device, fragShaderModule, nullptr);

}

struct RenderModel
{
    struct Vertex
    {
        Vec4 pos;
        Vec4 norm;
        Vec4 color;
    };
    struct AnimationVertexData
    {
        Vec4 weights;
        uint32_t indices[4];
    };

    std::vector<Vertex> vertices;
    std::vector<AnimationVertexData> animationVertices;
    std::vector<uint32_t> indices;
};

struct ModelBone
{

};

bool readGLTF(const char *filename, RenderModel &outModel)
{
    std::string fName = std::string(filename);
    PodVector<char> buffer;

    if(!loadBytes(fName, buffer))
        return false;

    JSONBlock bl;
    bool parseSuccess = bl.parseJSON(ArraySliceView(buffer.data(), buffer.size()));
    printf("parsed: %i\n", parseSuccess);
    if(!parseSuccess)
        return false;

    if(parseSuccess)
    {
//        bl.print();
    }


    struct SceneNode
    {
        std::string name;
        Quat rot;
        Vec3 trans;
        uint32_t meshIndex = ~0u;
        uint32_t skinIndex = ~0u;
        std::vector<uint32_t> childNodeIndices;
    };

    struct MeshNode
    {
        std::string name;
        uint32_t positionIndex = ~0u;
        uint32_t normalIndex = ~0u;
        uint32_t uvIndex = ~0u;
        uint32_t colorIndex = ~0u;
        uint32_t indicesIndex = ~0u;
        uint32_t materialIndex = ~0u;
        uint32_t jointsIndex = ~0u;
        uint32_t weightIndex = ~0u;
    };

    struct SkinNode
    {
        std::string name;
        std::vector<uint32_t> joints;
        uint32_t inverseMatricesIndex = ~0u;

        std::vector<Matrix> inverseMatrices;
    };

    std::vector<SceneNode> nodes;
    std::vector<MeshNode> meshes;
    std::vector<SkinNode> skins;
    std::vector<PodVector<uint8_t>> buffers;


    struct AnimationData
    {

    };

    if(!bl.isObject() || bl.getChildCount() < 1)
        return false;

    {
        const JSONBlock &meshBlock = bl.getChild("meshes");
        if(!meshBlock.isValid() || meshBlock.getChildCount() < 1)
            return false;

        meshes.resize(meshBlock.getChildCount());

        for(int i = 0; i < meshBlock.getChildCount(); ++i)
        {
            const JSONBlock &child = meshBlock.children [i];
            MeshNode &node = meshes [i];
            if(!child.getChild("name").parseString(node.name))
                return false;

            const JSONBlock &prims = child.getChild("primitives").getChild(0);
            if(!prims.isValid())
                return false;

            if(!prims.getChild("indices").parseUInt(node.indicesIndex) ||
                !prims.getChild("material").parseUInt(node.materialIndex))
                return false;

            const JSONBlock &attribs = prims.getChild("attributes");
            if(!attribs.isValid() || attribs.getChildCount() < 1)
                return false;

            if(!attribs.getChild("POSITION").parseUInt(node.positionIndex) ||
                !attribs.getChild("NORMAL").parseUInt(node.normalIndex) ||
                !attribs.getChild("COLOR_0").parseUInt(node.colorIndex))
                return false;

            attribs.getChild("TEXCOORD_0").parseUInt(node.uvIndex);

            attribs.getChild("JOINTS_0").parseUInt(node.jointsIndex);
            attribs.getChild("WEIGHTS_0").parseUInt(node.weightIndex);


        }
    }
    {
        const JSONBlock &nodeBlock = bl.getChild("nodes");
        if(!nodeBlock.isValid() || nodeBlock.getChildCount() < 1)
            return false;

        nodes.resize(nodeBlock.getChildCount());

        for(int i = 0; i < nodeBlock.getChildCount(); ++i)
        {
            const JSONBlock &child = nodeBlock.children [i];
            SceneNode &node = nodes [i];
            if(!child.getChild("name").parseString(node.name))
                return false;

            child.getChild("mesh").parseUInt(node.meshIndex);
            child.getChild("skin").parseUInt(node.skinIndex);


            const JSONBlock &rotBlock = child.getChild("rotation");
            rotBlock.getChild(0).parseNumber(node.rot.v.x);
            rotBlock.getChild(1).parseNumber(node.rot.v.y);
            rotBlock.getChild(2).parseNumber(node.rot.v.z);
            rotBlock.getChild(3).parseNumber(node.rot.w);

            const JSONBlock &transBlock = child.getChild("translation");
            transBlock.getChild(0).parseNumber(node.trans.x);
            transBlock.getChild(1).parseNumber(node.trans.y);
            transBlock.getChild(2).parseNumber(node.trans.z);


            const JSONBlock& childrenBlock = child.getChild("children");
            for (int childIndex = 0; childIndex < childrenBlock.getChildCount(); ++childIndex)
            {
                uint32_t tmpIndex = ~0u;
                if (!childrenBlock.getChild(childIndex).parseUInt(tmpIndex))
                    return false;
                node.childNodeIndices.push_back(tmpIndex);
            }

        }
    }

    {
        const JSONBlock& skinBlock = bl.getChild("skins");
        if (skinBlock.isValid() && skinBlock.getChildCount() > 0)
        {
            skins.resize(skinBlock.getChildCount());

            for (int i = 0; i < skinBlock.getChildCount(); ++i)
            {
                SkinNode& node = skins[i];
                const JSONBlock& child = skinBlock.children[i];

                if (!child.getChild("name").parseString(node.name))
                    return false;

                if (!child.getChild("inverseBindMatrices").parseUInt(node.inverseMatricesIndex))
                    return false;

                const JSONBlock& jointsBlock = child.getChild("joints");
                if (!jointsBlock.isValid() || jointsBlock.getChildCount() < 1)
                    return false;

                for (int j = 0; j < jointsBlock.getChildCount(); ++j)
                {
                    uint32_t tmpInt = ~0u;
                    if (!jointsBlock.getChild(j).parseUInt(tmpInt))
                        return false;

                    node.joints.push_back(tmpInt);
                }

            }
        }
    }

     {
        const JSONBlock &bufferBlock = bl.getChild("buffers");
        if(!bufferBlock.isValid() || bufferBlock.getChildCount() < 1)
            return false;

        buffers.resize(bufferBlock.getChildCount());

        for(int i = 0; i < bufferBlock.getChildCount(); ++i)
        {
            const JSONBlock &child = bufferBlock.children [i];
            PodVector<uint8_t> &buffer = buffers [i];

            uint32_t bufLen = 0u;
            if(!child.getChild("byteLength").parseUInt(bufLen))
                return false;

            if(!child.getChild("uri").parseBuffer(buffer))
                return false;

            if(bufLen != buffer.size())
                return false;
        }
    }

    {
        const JSONBlock &accessorBlock = bl.getChild("accessors");
        if(!accessorBlock.isValid() || accessorBlock.getChildCount() < 1)
            return false;

        const JSONBlock &bufferViewBlock = bl.getChild("bufferViews");
        if(!bufferViewBlock.isValid() || bufferViewBlock.getChildCount() < 1)
            return false;


        MeshNode &node = meshes [0];

        auto lam =[&](uint32_t index, int32_t floatStartOffsetIndex, bool useVertices)
        {
            if(index == ~0u || index >= accessorBlock.getChildCount())
                return false;

            const JSONBlock &block = accessorBlock.getChild(index);
            if(!block.isValid())
                return false;

            uint32_t viewIndex = ~0u;
            uint32_t componentType = ~0u;
            uint32_t count = ~0u;
            bool normalized = false;
            std::string s;

            if(!block.getChild("bufferView").parseUInt(viewIndex)
             || !block.getChild("componentType").parseUInt(componentType)
             || !block.getChild("count").parseUInt(count)
             || !block.getChild("type").parseString(s)
            )
                return false;

            if (block.getChild("sparse").isValid())
            {
                LOG("No sparse view are handled!\n");
                ASSERT(false && "No sparse view are handled");
                return false;
            }


            block.getChild("normalized").parseBool(normalized);

            uint32_t componentCount = ~0u;

            //"SCALAR"     1
            //"VEC2"     2
            //"VEC3"     3
            //"VEC4"     4
            //"MAT2"     4
            //"MAT3"     9
            //"MAT4"     16

            if (s == "SCALAR") componentCount = 1;
            else if (s == "VEC2") componentCount = 2;
            else if (s == "VEC3") componentCount = 3;
            else if (s == "VEC4") componentCount = 4;
            else if (s == "MAT2") componentCount = 4;
            else if (s == "MAT3") componentCount = 9;
            else if (s == "MAT4") componentCount = 16;
            else return false;

            //// Maybe 5124 is half?
            //5120 (BYTE)1
            //5121(UNSIGNED_BYTE)1
            //5122 (SHORT)2
            //5123 (UNSIGNED_SHORT)2
            //5125 (UNSIGNED_INT)4
            //5126 (FLOAT)4

            uint32_t componentTypeBitCount = 0u;
            switch(componentType)
            {
            case 5120:
            case 5121:
                componentTypeBitCount = 1u;
                break;

            case 5122:
            case 5123:
                componentTypeBitCount = 2u;
                break;

            case 5125:
            case 5126:
                componentTypeBitCount = 4u;
                break;

            default:
                return false;
            }
            uint32_t bufferIndex = ~0u;
            uint32_t bufferOffset = ~0u;
            uint32_t bufferLen = ~0u;

            const JSONBlock &bufferBlock = bufferViewBlock.getChild(viewIndex);
            if(!bufferBlock.isValid())
                return false;

            if(!bufferBlock.getChild("buffer").parseUInt(bufferIndex)
                || !bufferBlock.getChild("byteLength").parseUInt(bufferLen)
                || !bufferBlock.getChild("byteOffset").parseUInt(bufferOffset)
                )
                return false;

            if(bufferIndex >= buffers.size() || bufferOffset + bufferLen > buffers[bufferIndex].size() )
                return false;

            uint8_t *ptr = &buffers[bufferIndex][0] + bufferOffset;
            uint8_t *endPtr = &buffers[bufferIndex][0] + bufferOffset + bufferLen;


            // Doesnt exactly handle cases properly... Just reading stuff into float buffer, in case its either normalized u16 value or 32 bit float.
            if(useVertices)
            {
                bool isValidVertice = componentType == 5126 || (componentType == 5123 && normalized) || (componentType == 5121);
                ASSERT(isValidVertice);
                if (!isValidVertice)
                    return false;


                if(outModel.vertices.size() == 0)
                    outModel.vertices.resize(count);
                if(outModel.vertices.size() != count)
                    return false;

                for(uint32_t i = 0; i < count; ++i)
                {

                    RenderModel::Vertex &v = outModel.vertices[i];
                    float *f = (float *)(((uint8_t *)&v) + floatStartOffsetIndex);
                    for(uint32_t j = 0; j < componentCount; ++j)
                    {
                        if(ptr + componentTypeBitCount > endPtr)
                            return false;

                        float f1 = 0.0f;
                        uint32_t u1 = 0u;


                        if (componentType == 5126)
                        {
                            memcpy(&f1, ptr, componentTypeBitCount);
                        }
                        else if(componentType == 5123 && normalized)
                        {
                            uint16_t tmp = 0;
                            memcpy(&tmp, ptr, componentTypeBitCount);

                            f1 = (float)tmp / 65535.0f;
                        }
                        else
                        {
                            return false;
                        }


                        *(f + j) = f1;
                        ptr += componentTypeBitCount;
                    }
                }
            }
            // Assumption that all indices are either u16 or u32 values.
            else
            {
                bool isValidIndice = componentType == 5123 || componentType == 5125;
                ASSERT(isValidIndice);
                if (!isValidIndice)
                    return false;


                if(outModel.indices.size() != 0)
                    return false;
                outModel.indices.resize(count);

                for(uint32_t i = 0; i < count; ++i)
                {
                    if(ptr + componentTypeBitCount > endPtr)
                        return false;

                    uint32_t value = 0u;
                    if(componentTypeBitCount == 4)
                        memcpy(&value, ptr, componentTypeBitCount);
                    else if(componentTypeBitCount == 2)
                    {
                        uint16_t tmp = 0;
                        memcpy(&tmp, ptr, componentTypeBitCount);

                        value = tmp;
                    }
                    else
                        return false;

                    outModel.indices[i] = value;

                    ptr += componentTypeBitCount;
                }
            }
            return true;

        };

        if(!lam(node.positionIndex, offsetof(RenderModel::Vertex, pos), true))
            return false;

        if(!lam(node.normalIndex, offsetof(RenderModel::Vertex, norm), true))
            return false;

        if(!lam(node.colorIndex, offsetof(RenderModel::Vertex, color), true))
            return false;

        if(!lam(node.indicesIndex, 0, false))
            return false;
    }

/*
    for(uint32_t i = 0; i < vertices.size(); ++i)
    {
        printf("i: %i:   x: %f, y: %f, z: %f\n", i, vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z);
        printf("i: %i:   x: %f, y: %f, z: %f\n", i, vertices[i].norm.x, vertices[i].norm.y, vertices[i].norm.z);
        printf("i: %i:   x: %f, y: %f, z: %f, w: %f\n", i, vertices[i].color.x, vertices[i].color.y, vertices[i].color.z, vertices[i].color.w);
    }

    for(uint32_t i = 0; i < indices.size(); ++i)
    {
        printf("i: %i, index: %u\n", i, indices[i]);
    }
*/
    return true;
}

bool VulkanDrawStuff::init(const char* windowStr, int screenWidth, int screenHeight, const VulkanInitializationParameters& params)
{
    if (!VulkanApp::init(windowStr, screenWidth, screenHeight, params))
        return false;

    glfwSetWindowUserPointer(window, this);

    RenderModel renderModel;

    //bool readSuccess = readGLTF("assets/models/test_gltf.gltf", renderModel);
    //bool readSuccess = readGLTF("assets/models/arrows.gltf", renderModel);
    bool readSuccess = readGLTF("assets/models/animatedthing.gltf", renderModel);

    printf("gltf read success: %i\n", readSuccess);
    if (!readSuccess)
        return false;

    vertShaderModule = loadShader("assets/shader/vulkan_new/basic3d.vert.spv");
    ASSERT(vertShaderModule);

    fragShaderModule = loadShader("assets/shader/vulkan_new/basic3d.frag.spv");
    ASSERT(fragShaderModule);

    uniformDataHandle = vulk.uniformBufferManager.reserveHandle();

    quadBuffer = createBuffer(8u * 1024u * 1024u,
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
        offset = uploadToScratchbuffer(( void * )renderModel.indices.data(), size_t(sizeof(renderModel.indices[0]) * renderModel.indices.size()), offset);
        uploadScratchBufferToGpuBuffer(indexDataBuffer, offset);

        offset = 0;
        offset = uploadToScratchbuffer((void*)renderModel.vertices.data(), size_t(sizeof(renderModel.vertices[0]) * renderModel.vertices.size()), offset);
        uploadScratchBufferToGpuBuffer(quadBuffer, offset);


        indicesCount = renderModel.indices.size();
    }
    camera.position = Vec3(0.0f, 4.0f, 5.0f);

    return createPipelines();
}


bool VulkanDrawStuff::createPipelines()
{
    PipelineWithDescriptors &pipeline = graphicsPipeline;

    pipeline.descriptorSetLayouts = PodVector<DescriptorSetLayout>(
    {
        DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u },
        DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u },
        DescriptorSetLayout{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u },
    });

    if (!createPipelineLayout(pipeline, VK_SHADER_STAGE_ALL_GRAPHICS))
    {
        printf("Failed to create pipelinelayout!\n");
        return false;
    }

    pipeline.pipeline = createGraphicsPipeline(vulk.renderPass,
        vertShaderModule, fragShaderModule,
        pipeline.pipelineLayout, true);


    pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
        {
            DescriptorInfo(vulk.renderFrameBufferHandle),
            DescriptorInfo(uniformDataHandle),
            DescriptorInfo(quadBuffer.buffer, 0u, quadBuffer.size),
        });

    pipeline.descriptor = createDescriptor(pipeline.descriptorSetLayouts, pipeline.descriptorSetLayout);
    if (!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
    {
        printf("Failed to set descriptor binds!\n");
        return false;
    }
    return true;
}


void VulkanDrawStuff::update()
{
    VulkanApp::update();

    static uint32_t gpuframeCount = 0u;
    static double gpuTime = 0.0;
    static double cpuTimeStamp = glfwGetTime();


    MouseState mouseState = getMouseState();

    checkCameraKeypresses(dt, camera);
    ////////////////////////
    //
    // RENDER PASSES START
    // WRITING VALUES INTO
    // "CONSTANT BUFFEERS"
    //
    ////////////////////////
    SwapChain &swapchain = vulk.swapchain;

    struct FrameBuffer
    {
        Matrix camMat;
        Matrix viewProj;
        Matrix mvp;
        Matrix padding;
    };
    FrameBuffer b;


    b.camMat = camera.getCameraMatrix();

    camera.aspectRatioWByH = float(swapchain.width) / float(swapchain.height);
    camera.fovY = 90.0f;
    camera.zFar = 200.0f;
    camera.zNear = 0.001f;

    b.viewProj = camera.perspectiveProjectionRH();
    b.mvp = b.camMat * b.viewProj;

    Transform trans;
    trans.pos = Vec3(3.0f, 3.0f, 13.0f);
    static float rotationAmount = Pi * 0.25f;

    trans.rot = getQuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), rotationAmount);
    Vec3 tmp = rotateVector(Vector3(0.0f, 0.0f, 1.0f), trans.rot);
    trans.scale = Vec3(1.0f, 1.0f, 1.0f);

    Transform trans2;
    trans2.pos = Vec3(5.0f, 0.0f, 0.0f);
    rotationAmount += 1.5f * dt;

    //b.padding = getModelMatrix(trans); // *getModelMatrix(trans);

    camera.renderCameraInfo(fontSystem, Vec2(10.0f, 10.0f), Vec2(8.0f, 12.0f));

    if (!startRender(window))
        return;

    beginSingleTimeCommands();
    vkCmdResetQueryPool(vulk.commandBuffer, vulk.queryPool, 0, QUERY_COUNT);
    vkCmdWriteTimestamp(vulk.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vulk.queryPool, TIME_POINTS::START_POINT);


    {

        updateRenderFrameBuffer();
        uint32_t offset = vulk.scratchBufferOffset;
        // use scratch buffer to unifrom buffer transfer
        uint32_t bufSize = sizeof(FrameBuffer);
        memcpy(( void * )( ( char * )vulk.scratchBuffer.data + offset ), &b, bufSize);

        {
            VkBufferCopy region = { offset, uniformDataHandle.getOffset(), VkDeviceSize(bufSize)};
            vkCmdCopyBuffer(vulk.commandBuffer, vulk.scratchBuffer.buffer, vulk.uniformBuffer.buffer, 1, &region);
        }

        VkBufferMemoryBarrier bar[ ]
        {
            bufferBarrier(vulk.uniformBuffer.buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, bufSize),
        };

        vkCmdPipelineBarrier(vulk.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZES(bar), bar, 0, nullptr);

        vulk.scratchBufferOffset += bufSize;
    }



    ////////////////////////
    //
    // MAIN RENDER
    //
    ////////////////////////
    {
        VkImageMemoryBarrier imageBarriers[ ] =
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
        VkClearValue clearValues [2] = { };
        clearValues [0].color = VkClearColorValue { { 0.0f, 0.5f, 1.0f, 1.0f } };
        clearValues [1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        passBeginInfo.renderPass = vulk.renderPass;
        passBeginInfo.framebuffer = vulk.targetFB;
        passBeginInfo.renderArea.extent.width = swapchain.width;
        passBeginInfo.renderArea.extent.height = swapchain.height;
        passBeginInfo.clearValueCount = ARRAYSIZES(clearValues);
        passBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(vulk.commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewPort = { 0.0f, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.0f, 1.0f };
        VkRect2D scissors = { { 0, 0 }, { uint32_t(swapchain.width), uint32_t(swapchain.height) } };

        insertDebugRegion("Render", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        vkCmdSetViewport(vulk.commandBuffer, 0, 1, &viewPort);
        vkCmdSetScissor(vulk.commandBuffer, 0, 1, &scissors);


        // draw calls here
        // Render
        {
            bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            vkCmdBindIndexBuffer(vulk.commandBuffer, indexDataBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulk.commandBuffer, indicesCount, 1, 0, 0, 0);

        }

        fontSystem.render();

        vkCmdEndRenderPass(vulk.commandBuffer);
    }

    vkCmdWriteTimestamp(vulk.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, vulk.queryPool, TIME_POINTS::DRAW_FINISHED);

    present(window);

    ////////////////////////
    //
    // END PASS, COLLECT TIMINGS
    //
    ////////////////////////


    uint64_t queryResults [TIME_POINTS::NUM_TIME_POINTS];
    vkGetQueryPoolResults(vulk.device, vulk.queryPool, 0, ARRAYSIZES(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults [0]), VK_QUERY_RESULT_64_BIT);


    struct TimeValues
    {
        double timeDuration [TIME_POINTS::NUM_TIME_POINTS];
    };

    VkPhysicalDeviceProperties props = { };
    vkGetPhysicalDeviceProperties(vulk.physicalDevice, &props);

    static TimeValues timeValues = { };
    for(uint32_t i = TIME_POINTS::NUM_TIME_POINTS - 1; i > 0; --i)
        timeValues.timeDuration [i] += ( double(queryResults [i]) - double(queryResults [i - 1]) ) * props.limits.timestampPeriod * 1.0e-9f;

    gpuTime += ( double(queryResults [TIME_POINTS::NUM_TIME_POINTS - 1]) - double(queryResults [0]) ) * props.limits.timestampPeriod * 1.0e-9f;

    ++gpuframeCount;
    if(glfwGetTime() - cpuTimeStamp >= 1.0)
    {
        double d = 1000.0 / gpuframeCount;
        double e = gpuframeCount;
        double currTime = glfwGetTime();
        double cpuTime = currTime - cpuTimeStamp;
        cpuTimeStamp += 1.0f;

        printf("Gpu: %.3fms, cpu: %.3fms, draw: %.3fms. GpuFps:%.1f, CpuFps:%.1f\n",
            ( float )( gpuTime * d ), ( float )( cpuTime * d ),
            ( float )( timeValues.timeDuration [DRAW_FINISHED] * d ),
            e / gpuTime, e / cpuTime);
        gpuframeCount = 0u;

        for(uint32_t i = 0; i < TIME_POINTS::NUM_TIME_POINTS; ++i)
            timeValues.timeDuration [i] = 0.0;

        gpuTime = 0.0;
    }


    char str [100];
    float fps = dt > 0.0 ? float(1.0 / dt) : 0.0f;
    sprintf(str, "%2.2fms, fps: %4.2f, mx: %i, my: %i, ml: %i, mr: %i, mb: %i",
        float(dt * 1000.0), fps,
        mouseState.x, mouseState.y, mouseState.leftButtonDown, mouseState.rightButtonDown, mouseState.middleButtonDown);
    setTitle(str);
}





int main(int argCount, char **argv)
{
    VulkanDrawStuff app;
    if (app.init("Vulkan, render font", SCREEN_WIDTH, SCREEN_HEIGHT,
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