#include <assert.h>
#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <set>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <unordered_set>



#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include <core/vulkan_app.h>
#include <core/general.h>

#include <core/timer.h>
#include <core/transform.h>
#include <core/mytypes.h>

#include "myvulkan/vulkandevice.h"
#include "myvulkan/vulkanhelperfuncs.h"
#include "myvulkan/vulkanresource.h"
#include "myvulkan/vulkanshader.h"
#include "myvulkan/vulkanswapchain.h"

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>
 // testign22


#include "model.h"
#include <core/camera.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "meshoptimizer/src/meshoptimizer.h"


enum TIME_POINTS
{
    START_POINT,
    COMPUTE_BEFORE_TRIANGLE_BUILD,
    COMPUTE_FINISHED,
    DRAW_FINISHED,
    COPY_FINISHED,

    NUM_TIME_POINTS
};





#if DISCRETE_GPU
    const u32 drawCount = 250;
#else
    const u32 drawCount = 1;
#endif

enum MeshTypes
{
    MESH_KITTEN,
    MESH_HUGE,

    NUM_MESH_TYPES
};




struct MeshTypeData
{
    u32 vertexStartIndex = ~0u;
    u32 vertexAmount = 0u;
    u32 indiceStartIndex = ~0u;
    u32 indiceAmount = 0u;
    u32 meshletStartIndex = ~0u;
    u32 meshletAmount = 0u;
    u32 padding1 = 0u;
    u32 padding2 = 0u;
};




struct MeshDatas
{
    u32 indiceCount = 0u;
};

//


enum ShaderModuleIndexes
{
    SHADER_MODULE_COMPUTE_RESET,
    SHADER_MODULE_COMPUTE_MESH_BUILD,
    SHADER_MODULE_COMPUTE_TRIANGLE_BUILD,

    SHADER_MODULE_GRAPHICS_RENDER_VERT,
    SHADER_MODULE_GRAPHICS_RENDER_FRAG,

    SHADER_MODULE_COMPUTE_CARP_WRITE_NUMBER,

    NUM_SHADER_MODULES
};

// Probably not good in long run?
enum PipelineIndexes
{
    PIPELINE_COMPUTE_RESET,
    PIPELINE_COMPUTE_MESHES_BUILD,
    PIPELINE_COMPUTE_TRIANGLE_BUILD,

    PIPELINE_GRAPHICS_PIPELINE,

    PIPELINE_COMPUTE_CARP_WRITE_NUMBER,

    NUM_PIPELINE
};

enum BufferIndexes
{
    UNIFORM_BUFFER,

    VERTEX_DATA_BUFFER,
    INDEX_DATA_BUFFER,
    MESHLET_DATA_BUFFER,
    MESH_INSTANCE_DATA_BUFFER,

    INSTANCE_INDEX_BUFFER,

    INDEX_WRITE_BUFFER,
    VERTEX_WRITE_BUFFER,

    ATOMIC_BUFFER,

    NUM_BUFFERS
};


enum RenderTargetImageIndexes
{
    MAIN_COLOR_TARGET,
    MAIN_DEPTH_TARGET,

    NUM_TARGET_IMAGES
};






class VulkanTest : public VulkanApp
{
public:
    VulkanTest() {}
    virtual ~VulkanTest() override;
    virtual bool init(const char *windowStr, int screenWidth, int screenHeight) override;
    virtual void run() override;

    void checkKeypresses(float deltaTime, Camera &camera);

public:
    void recreateSwapchainData() override;

    VkShaderModule shaderModules[NUM_SHADER_MODULES] = {};
    Buffer buffers[NUM_BUFFERS];

    Image renderTargetImages[NUM_TARGET_IMAGES];
    std::vector<DescriptorSet> descriptorSets[NUM_SHADER_MODULES];


    Pipeline pipelines[NUM_PIPELINE];

    VkSampler mainImageComputeWriteSampler = 0; // can write without sampler?

    MeshDatas meshData;
    MeshTypeData meshTypeData[NUM_MESH_TYPES];
};






struct alignas(16) MeshDraw
{
    float offset[2];
    float scale[2];
};





struct MeshInstance
{
    u32 instanceTypeIndex = 0u;
};










////////////////////////
//
// DEINIT
//
////////////////////////

VulkanTest::~VulkanTest()
{
    VkDevice device = deviceWithQueues.device;

    for (auto& image : renderTargetImages)
        destroyImage(device, image);

    for (auto& pipeline : pipelines)
    {
        destroyDescriptor(device, pipeline.descriptor);
        destroyPipeline(device, pipeline.pipeline);
    }

    vkDestroySampler(device, mainImageComputeWriteSampler, nullptr);

    for (auto& buffer : buffers)
        destroyBuffer(device, buffer);

    for (auto& shaderModule : shaderModules)
        vkDestroyShaderModule(device, shaderModule, nullptr);

}



bool VulkanTest::init(const char *windowStr, int screenWidth, int screenHeight)
{
    if(!VulkanApp::init(windowStr, screenWidth, screenHeight))
        return false;

    glfwSetWindowUserPointer(window, this);


    // Threaded loads?
    Mesh meshes[NUM_MESH_TYPES];

    {
        const char* modelNames[] =
        {
            "assets/models/kitten.obj",
            "assets/models/kitten.obj",
            //"assets/models/chalet.obj"
        };

        static_assert(ARRAYSIZES(modelNames) == NUM_MESH_TYPES);
        size_t totalMeshIndices = 0;

        for (u32 i = 0; i < NUM_MESH_TYPES; ++i)
        {
            Mesh &mesh = meshes[i];
            [[maybe_unused]] bool loading = false;
            loading = loadMesh(mesh, modelNames[i], totalMeshIndices);
            ASSERT(loading);
            totalMeshIndices += (u32)mesh.indices.size();
            ASSERT(totalMeshIndices < (1u << 20u));
        }
    }
    meshData.indiceCount = u32(meshes[MESH_KITTEN].indices.size());


    VkDevice device = deviceWithQueues.device;
    shaderModules[SHADER_MODULE_COMPUTE_RESET] = loadShader(device, "assets/shader/vulkan/computereset_comp.spv");
    ASSERT(shaderModules[SHADER_MODULE_COMPUTE_RESET]);

    shaderModules[SHADER_MODULE_COMPUTE_MESH_BUILD] = loadShader(device, "assets/shader/vulkan/compute_create_instances_comp.spv"); //compute_comp.spv");
    ASSERT(shaderModules[SHADER_MODULE_COMPUTE_MESH_BUILD]);

    shaderModules[SHADER_MODULE_COMPUTE_TRIANGLE_BUILD] = loadShader(device, "assets/shader/vulkan/compute_comp.spv"); //compute_comp.spv");
    ASSERT(shaderModules[SHADER_MODULE_COMPUTE_TRIANGLE_BUILD]);

    shaderModules[SHADER_MODULE_GRAPHICS_RENDER_VERT] = loadShader(device, "assets/shader/vulkan/triangle_vert.spv");
    ASSERT(shaderModules[SHADER_MODULE_GRAPHICS_RENDER_VERT]);

    shaderModules[SHADER_MODULE_GRAPHICS_RENDER_FRAG] = loadShader(device, "assets/shader/vulkan/triangle_frag.spv");
    ASSERT(shaderModules[SHADER_MODULE_GRAPHICS_RENDER_FRAG]);

    shaderModules[SHADER_MODULE_COMPUTE_CARP_WRITE_NUMBER] = loadShader(device, "assets/shader/vulkan/computecarpwrite_comp.spv");
    ASSERT(shaderModules[SHADER_MODULE_COMPUTE_CARP_WRITE_NUMBER]);




    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);


    buffers[UNIFORM_BUFFER] = createBuffer(device, memoryProperties, 64u * 1024,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Uniform buffer");


    //buffers[VERTEX_DATA_BUFFER] = createBuffer(device, memoryProperties, 128 * 1024 * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    //buffers[INDEX_DATA_BUFFER] = createBuffer(device, memoryProperties, 128 * 1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    buffers[VERTEX_DATA_BUFFER] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
        /*VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |*/ VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex data buffer");
    buffers[INDEX_DATA_BUFFER] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
        /*VK_BUFFER_USAGE_INDEX_BUFFER_BIT |*/ VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");
    buffers[MESHLET_DATA_BUFFER] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Meshlet data buffer");
    #if DISCRETE_GPU
        buffers[INDEX_WRITE_BUFFER] = createBuffer(device, memoryProperties, 256 * 1024 * 1024,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index write buffer");
        buffers[VERTEX_WRITE_BUFFER] = createBuffer(device, memoryProperties, 256 * 1024 * 1024,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex write buffer");
    #else
        buffers[INDEX_WRITE_BUFFER] = createBuffer(device, memoryProperties, 128 * 1024 * 1024,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index copy buffer");
        buffers[VERTEX_WRITE_BUFFER] = createBuffer(device, memoryProperties, 128 * 1024 * 1024,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex write buffer");
    #endif
    buffers[ATOMIC_BUFFER] = createBuffer(device, memoryProperties, 1 * 64u * 1024,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Atomic buffer");


    buffers[MESH_INSTANCE_DATA_BUFFER] = createBuffer(device, memoryProperties, 1 * 64u * 1024,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Mesh instance index data buffer");
    buffers[INSTANCE_INDEX_BUFFER] = createBuffer(device, memoryProperties, 1024u * 64u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Instance index buffer");



    // Random tag data
    //struct DemoTag { const char name[17] = "debug marker tag"; } demoTag;
    //setObjectTag(device, (uint64_t)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);


    MeshTypeData meshTypeData[NUM_MESH_TYPES]{};

    // Upload mesh datas
    {
        size_t offset = 0;
        for (u32 i = 0; i < NUM_MESH_TYPES; ++i)
        {
            const Mesh& mesh = meshes[i];
            meshTypeData[i].vertexStartIndex = u32(offset);
            offset = uploadToScratchbuffer(scratchBuffer, (void*)mesh.vertices.data(), size_t(sizeof(mesh.vertices[0]) * mesh.vertices.size()), offset);
            meshTypeData[i].vertexAmount = u32(mesh.vertices.size());
        }
        uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
            buffers[VERTEX_DATA_BUFFER], scratchBuffer, offset);

        offset = 0;
        for (u32 i = 0; i < NUM_MESH_TYPES; ++i)
        {
            const Mesh& mesh = meshes[i];
            meshTypeData[i].indiceStartIndex = u32(offset);
            offset = uploadToScratchbuffer(scratchBuffer, (void*)mesh.indices.data(), size_t(sizeof(mesh.indices[0]) * mesh.indices.size()), offset);
            meshTypeData[i].indiceAmount = u32(mesh.indices.size());
        }

        uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
            buffers[INDEX_DATA_BUFFER], scratchBuffer, offset);

        offset = 0;
        for (u32 i = 0; i < NUM_MESH_TYPES; ++i)
        {
            const Mesh& mesh = meshes[i];
            meshTypeData[i].meshletStartIndex = u32(offset);
            offset = uploadToScratchbuffer(scratchBuffer, (void*)mesh.meshlets.data(), size_t(sizeof(mesh.meshlets[0]) * mesh.meshlets.size()), offset);
            meshTypeData[i].meshletAmount = u32(mesh.meshlets.size());
        }
        uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
            buffers[MESHLET_DATA_BUFFER], scratchBuffer, offset);
    }


    // upload mesh instance data
    {
        size_t offset = 0;
        std::vector<u32> meshTypeIndexes(drawCount);
        for (u32 i = 0; i < drawCount; ++i)
        {
            meshTypeIndexes[i] = u32(rand()) % NUM_MESH_TYPES;
        }

        offset = uploadToScratchbuffer(scratchBuffer, (void*)meshTypeData, size_t(sizeof(meshTypeData)), 0);
        offset = uploadToScratchbuffer(scratchBuffer, (void*)meshTypeIndexes.data(), u32(meshTypeIndexes.size() * sizeof(u32)), offset);

        uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
            buffers[MESH_INSTANCE_DATA_BUFFER], scratchBuffer, offset);
    }



    // create color and depth images
    {
        renderTargetImages[MAIN_COLOR_TARGET] = createImage(device, deviceWithQueues.queueFamilyIndices.graphicsFamily, memoryProperties,
            swapchain.width, swapchain.height, deviceWithQueues.computeColorFormat,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            "Main color target image");
        renderTargetImages[MAIN_DEPTH_TARGET] = createImage(device, deviceWithQueues.queueFamilyIndices.graphicsFamily, memoryProperties,
            swapchain.width, swapchain.height, deviceWithQueues.depthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            "Main depth target image");
        targetFB = createFramebuffer(device, renderPass,
            renderTargetImages[MAIN_COLOR_TARGET].imageView, renderTargetImages[MAIN_DEPTH_TARGET].imageView,
            swapchain.width, swapchain.height);
    }






    //VkDescriptorUpdateTemplate meshUpdateTemplate = createUpdateTemplate(device, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout);
    //ASSERT(meshUpdateTemplate);

    struct HelperStruct
    {
        static void setGraphicsPipeline(VkDevice device, Pipeline &pipelineWithDescriptors, VulkanTest *test,
            ShaderModuleIndexes vertShaderIndex, ShaderModuleIndexes fragShaderIndex, const VertexInput &vertexInput,
            u32 pushConstantSize = 0u, VkShaderStageFlagBits pushConstantStageUsage = VK_SHADER_STAGE_ALL_GRAPHICS)
        {
            pipelineWithDescriptors.pipeline = createGraphicsPipeline(device, test->renderPass, test->pipelineCache,
                test->shaderModules[vertShaderIndex], test->shaderModules[fragShaderIndex], vertexInput, pipelineWithDescriptors.descriptorSet, true,
                pushConstantSize, pushConstantStageUsage);
            pipelineWithDescriptors.descriptor = createDescriptor(device, pipelineWithDescriptors.descriptorSet, pipelineWithDescriptors.pipeline.descriptorSetLayout);
        }

        static void setComputePipeline(VkDevice device, Pipeline &pipelineWithDescriptors, VulkanTest *test,
            ShaderModuleIndexes computeShaderIndex,
            u32 pushConstantSize = 0u, VkShaderStageFlagBits pushConstantStageUsage = VK_SHADER_STAGE_ALL_GRAPHICS)
        {
            pipelineWithDescriptors.pipeline = createComputePipeline(device, test->pipelineCache, test->shaderModules[computeShaderIndex],
                pipelineWithDescriptors.descriptorSet, pushConstantSize, pushConstantStageUsage);
            pipelineWithDescriptors.descriptor = createDescriptor(device, pipelineWithDescriptors.descriptorSet, pipelineWithDescriptors.pipeline.descriptorSetLayout);

        }
    };


    {
        Pipeline &pipeline = pipelines[PIPELINE_GRAPHICS_PIPELINE];
        VertexInput vertexInput;
        //vertexInput.formats.push_back(VK_FORMAT_R32G32B32_SFLOAT);
        //vertexInput.formats.push_back(VK_FORMAT_R32G32B32_SFLOAT);
        //vertexInput.formats.push_back(VK_FORMAT_R32G32_SFLOAT);

        pipeline.descriptorSet = std::vector<DescriptorSet>(
        {
            DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &buffers[UNIFORM_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, true, &buffers[VERTEX_DATA_BUFFER] },
            //DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, true, &indexCopyBuffer };
            //DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3u, true, &atomicBuffer };
            DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 8u, true, &buffers[VERTEX_WRITE_BUFFER] },
        });

        HelperStruct::setGraphicsPipeline(device, pipeline, this, SHADER_MODULE_GRAPHICS_RENDER_VERT, SHADER_MODULE_GRAPHICS_RENDER_FRAG, vertexInput,
            u32(sizeof(MeshDraw)), VK_SHADER_STAGE_ALL_GRAPHICS);
    }

    {
        Pipeline& pipeline = pipelines[PIPELINE_COMPUTE_MESHES_BUILD];
        pipeline.descriptorSet = std::vector<DescriptorSet>(
            {
                DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &buffers[UNIFORM_BUFFER] },

                DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3u, true, &buffers[INDEX_DATA_BUFFER] },
                DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4u, true, &buffers[ATOMIC_BUFFER] },

                DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6u, true, &buffers[MESH_INSTANCE_DATA_BUFFER] },
                DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7u, true, &buffers[INSTANCE_INDEX_BUFFER] },
            });
        HelperStruct::setComputePipeline(device, pipeline, this, SHADER_MODULE_COMPUTE_MESH_BUILD);

    }
    {
        Pipeline &pipeline = pipelines[PIPELINE_COMPUTE_TRIANGLE_BUILD];
        pipeline.descriptorSet = std::vector<DescriptorSet>(
        {
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &buffers[UNIFORM_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, true, &buffers[VERTEX_DATA_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, true, &buffers[INDEX_WRITE_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3u, true, &buffers[INDEX_DATA_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4u, true, &buffers[ATOMIC_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5u, true, &buffers[MESHLET_DATA_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6u, true, &buffers[MESH_INSTANCE_DATA_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7u, true, &buffers[INSTANCE_INDEX_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 8u, true, &buffers[VERTEX_WRITE_BUFFER] },
            });
        HelperStruct::setComputePipeline(device, pipeline, this, SHADER_MODULE_COMPUTE_TRIANGLE_BUILD);
    }



    {
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE; //VK_TRUE;
        samplerInfo.maxAnisotropy = 1;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.minLod = 0;
        samplerInfo.maxLod = 1.0f;
        samplerInfo.mipLodBias = 0;

        VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &mainImageComputeWriteSampler));
    }

    {
        Pipeline &pipeline = pipelines[PIPELINE_COMPUTE_CARP_WRITE_NUMBER];
        pipeline.descriptorSet = std::vector<DescriptorSet>(
        {
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &buffers[UNIFORM_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, true, &buffers[INDEX_WRITE_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4u, true, &buffers[ATOMIC_BUFFER] },
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6u, true, nullptr, 0u, 0u,
                renderTargetImages[MAIN_COLOR_TARGET].image, renderTargetImages[MAIN_COLOR_TARGET].imageView, mainImageComputeWriteSampler, VK_IMAGE_LAYOUT_GENERAL},
        });
        HelperStruct::setComputePipeline(device, pipeline, this, SHADER_MODULE_COMPUTE_CARP_WRITE_NUMBER);
    }

    {
        Pipeline &pipeline = pipelines[PIPELINE_COMPUTE_RESET];
        pipeline.descriptorSet = std::vector<DescriptorSet>(
        {
            DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4u, true, &buffers[ATOMIC_BUFFER] },
        });
        HelperStruct::setComputePipeline(device, pipeline, this, SHADER_MODULE_COMPUTE_RESET);
    }

    return true;
}




void VulkanTest::recreateSwapchainData()
{
    VkDevice device = deviceWithQueues.device;


    vkDestroyFramebuffer(device, targetFB, nullptr);
    destroyImage(device, renderTargetImages[MAIN_COLOR_TARGET]);
    destroyImage(device, renderTargetImages[MAIN_DEPTH_TARGET]);

    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
    deviceWithQueues.queueFamilyIndices = queueFamilyIndices;
    ASSERT(deviceWithQueues.queueFamilyIndices.isValid());

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    renderTargetImages[MAIN_COLOR_TARGET] = createImage(device, queueFamilyIndices.graphicsFamily, memoryProperties,
        swapchain.width, swapchain.height, deviceWithQueues.colorFormat,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Main color target image");
    renderTargetImages[MAIN_DEPTH_TARGET] = createImage(device, queueFamilyIndices.graphicsFamily, memoryProperties,
        swapchain.width, swapchain.height, deviceWithQueues.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Main depth target image");
    targetFB = createFramebuffer(device, renderPass,
        renderTargetImages[MAIN_COLOR_TARGET].imageView, renderTargetImages[MAIN_DEPTH_TARGET].imageView, swapchain.width, swapchain.height);
    needToResize = false;

    // Should probably handle descriptor pool reseting instead of actually destroying it.
    {
        Pipeline &pipelineWithDescriptors = pipelines[PIPELINE_COMPUTE_CARP_WRITE_NUMBER];
        destroyDescriptor(device, pipelineWithDescriptors.descriptor);

        pipelineWithDescriptors.descriptorSet[3].image = renderTargetImages[MAIN_COLOR_TARGET].image;
        pipelineWithDescriptors.descriptorSet[3].imageView = renderTargetImages[MAIN_COLOR_TARGET].imageView;
        pipelineWithDescriptors.descriptor = createDescriptor(device, pipelineWithDescriptors.descriptorSet, pipelineWithDescriptors.pipeline.descriptorSetLayout);
    }
}











struct UniformValues
{
    //float transforms[128 * 16];
    float transforms[1200 * 12] = {};
    //glm::mat4 transforms[512];
    Matrix mvp;
    Matrix camMat;

    Vec4 cameraForwardDir; // padded
    Vec4 cameraPosition; // padded
    Vec4 screenSize; // padded

    Vec4 cameraPerspectionAdding; // single value

    Vec4 frustumPlanes[6];
    u32 someValues[4];
    Vec4 padding2;

    Alphabet alphabets[256] = {};


};


struct DeferDelete
{
    DeferDelete(UniformValues* ptr) { pointer = ptr; }
    ~DeferDelete() { delete(pointer); }

    UniformValues *pointer = nullptr;
};


////////////////////////
//
// MAIN START
//
////////////////////////

int main(int argc, char **argv)
{
    VulkanTest app;
    if(app.init("Test", 1024, 768))
        app.run();
    else
        return 1;

    return 0;
}




void VulkanTest::run()
{
    u32 frameIndex = 0u;

    Camera camera;

    std::vector <Transform> meshTransforms(drawCount);
    std::vector<MeshDraw> draws(drawCount);

    // Moved out of stack because visual studio doesnt like over 16k stack for a function.
    UniformValues *memoryUniform = new UniformValues();
    DeferDelete deferDeleteUniformMemory = DeferDelete(memoryUniform);
    UniformValues &uniformValues = *memoryUniform;


    ASSERT(drawCount <= sizeof(uniformValues.transforms) / (sizeof(float) * 12));

    generateAlphabets(uniformValues.alphabets);

    Vec3 dirs[drawCount];

    static constexpr float SpreadDist = 5.0f;

    for(u32 i = 0; i < drawCount; ++i)
    {
        Transform &transform = meshTransforms[i];
        transform.scale = Vec3(1.0f, 1.0f, 1.0f);
        transform.pos = Vec3(i, i, 15.0f - i * 3);
        transform.pos.x = rand() / float(RAND_MAX) * 2.0f * SpreadDist - SpreadDist;
        transform.pos.y = rand() / float(RAND_MAX) * 2.0f * SpreadDist - SpreadDist;
        transform.pos.z = rand() / float(RAND_MAX) * 2.0f * SpreadDist + SpreadDist;

        Vec3 rotationVector;
        rotationVector.x = (rand() / float(RAND_MAX) * 2.0f - 1.0f);
        rotationVector.y = (rand() / float(RAND_MAX) * 2.0f - 1.0f);
        rotationVector.z = (rand() / float(RAND_MAX) * 2.0f - 1.0f);
        float angle = rand() / float(RAND_MAX) * pii * 2.0f;
        transform.rot = getQuaternionFromAxisAngle(normalize(rotationVector), angle);

        //glm::mat4 modelMatrix = glm::transpose(getModelMatrix(transform));
        //uniformValues.transforms[i] = modelMatrix;

        Matrix modelMatrix = transpose(getModelMatrix(transform));
        Supa::memcpy(&uniformValues.transforms[i * 12], (float*)(&modelMatrix), sizeof(float) * 12);

        float newangle = rand() / float(RAND_MAX) * pii * 2.0f;

        Vec3 dir;
        dir.x = rand() / float(RAND_MAX) * 2.0f - 1.0f;
        dir.y = rand() / float(RAND_MAX) * 2.0f - 1.0f;
        dir.z = rand() / float(RAND_MAX) * 2.0f - 1.0f;

        dir = normalize(dir);

        Quat rotation = getQuaternionFromAxisAngle(dir, newangle);
        dirs[i] = rotateVector(Vec3(0.0f, 0.0f, 1.0f), rotation);
        //dirs[i] = Vec3(0.0f, 0.0f, 0.0f);
/*
        draws[i].offset[0] = float(i % drawCountSingleDir) / 7.0f + 1.0f / 14.0f;
        draws[i].offset[1] = float(i / drawCountSingleDir) / 7.0f + 1.0f / 14.0f;

        draws[i].scale[0] = 1.0f / 10.0f;
        draws[i].scale[1] = 1.0f / 10.0f;
        */
    }
















    ////////////////////////
    //
    // MAIN LOOP START
    // UPDATING ENGINE STATE
    //
    ////////////////////////

    float previousFrameTime = (float)glfwGetTime();
    u32 framesSinceLastDelta = 0u;
    float deltaTime = 0.0f;

    u32 gpuframeCount = 0u;
    double gpuTime = 0.0;
    double cpuTimeStamp = glfwGetTime();


    VkDevice device = deviceWithQueues.device;

    while(!glfwWindowShouldClose(window))
    {
        if(++framesSinceLastDelta >  10)
        {
            float newTime = (float)glfwGetTime();
            deltaTime = (newTime - previousFrameTime) / framesSinceLastDelta;
            previousFrameTime = newTime;
            framesSinceLastDelta = 0u;
        }
        ++frameIndex;
        glfwPollEvents();
        checkCameraKeypresses(deltaTime, camera);

        if (!startRender())
            continue;






        ////////////////////////
        //
        // RENDER PASSES START
        // WRITING VALUES INTO
        // "CONSTANT BUFFEERS"
        //
        ////////////////////////


        beginSingleTimeCommands(device, commandPool, commandBuffer);

        vkCmdResetQueryPool(commandBuffer, queryPool, 0, QUERY_COUNT);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, TIME_POINTS::START_POINT);



        // Update uniform values and shove them into gpu
        {
            uniformValues.screenSize.x = (float)swapchain.width;
            uniformValues.screenSize.y = (float)swapchain.height;

            uniformValues.screenSize.z = (float)(frameIndex % 1024);
            uniformValues.screenSize.w = (float)(frameIndex % 1024);

            uniformValues.camMat = createMatrixFromLookAt(camera.position, camera.position + camera.forwardDir, camera.upDir);

            camera.aspectRatioWByH = float(swapchain.width) / float(swapchain.height);
            camera.fovY = 90.0f;
            camera.zFar = 0.1f;

            uniformValues.mvp = perspectiveProjection(camera) * uniformValues.camMat;
            uniformValues.cameraPosition = Vec4(camera.position, 0.0f);
            uniformValues.cameraForwardDir = Vec4(camera.forwardDir, 0.0f);


            float fovY = toRadians(camera.fovY);
            float f = 1.0f / tanf(fovY / 2.0f);
            float divider = std::max(1.0e-9f, std::min(1.0f, f));
            float cosAngle = dot(Vec3(0.0f, 0.0f, 1.0f), normalize(Vec3(1.0f / divider, 1.0f, 1.0f)));
            uniformValues.cameraPerspectionAdding.x = cosAngle;
            uniformValues.cameraPerspectionAdding.y = Supa::acos(cosAngle);
            uniformValues.someValues[0] = drawCount;
            //printf("Cos angle: %f\n", glm::degrees(Supa::acos(cosAngle)));
            for(u32 i = 0; i < drawCount; ++i)
            {
                Transform &transform = meshTransforms[i];

                transform.pos = transform.pos + (dirs[i] * deltaTime * 10.0f);

                Matrix modelMatrix = transpose(getModelMatrix(transform));
                Supa::memcpy(&uniformValues.transforms[i * 12], (float*)(&modelMatrix), sizeof(float) * 12);

                if(transform.pos.x > 20.0f|| transform.pos.x < -20.0f)
                {
                    transform.pos.x = Supa::clampf(transform.pos.x, -20.0f, 20.0f);
                    dirs[i].x = -dirs[i].x;
                }
                if(transform.pos.y > 20.0f|| transform.pos.y < -20.0f)
                {
                    transform.pos.y = Supa::clampf(transform.pos.y, -20.0f, 20.0f);
                    dirs[i].y = -dirs[i].y;
                }
                if(transform.pos.z > 80.0f|| transform.pos.z < 10.0f)
                {
                    transform.pos.z = Supa::clampf(transform.pos.z, 10.0f, 80.0f);
                    dirs[i].z = -dirs[i].z;
                }
            }


            // Copy to uniform buffer
            {
                // use scratch buffer to unifrom buffer transfer
                Supa::memcpy(scratchBuffer.data, &uniformValues, sizeof(UniformValues));

                VkBufferCopy region = { 0, 0, VkDeviceSize(sizeof(UniformValues)) };
                vkCmdCopyBuffer(commandBuffer, scratchBuffer.buffer, buffers[UNIFORM_BUFFER].buffer, 1, &region);

                VkBufferMemoryBarrier copyBarrier = bufferBarrier(buffers[UNIFORM_BUFFER].buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, sizeof(UniformValues));
                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyBarrier, 0, nullptr);


                /*
                    void *data = nullptr;
                    VK_CHECK(vkMapMemory(device, buffers[UNIFORM_BUFFER].deviceMemory, 0, sizeof(UniformValues), 0, &data));
                    Supa::memcpy(buffers[UNIFORM_BUFFER].data, &uniformValues, sizeof(UniformValues));
                    vkUnmapMemory(device, buffers[UNIFORM_BUFFER].deviceMemory);
                    VkBufferMemoryBarrier bar[]
                    {
                        bufferBarrier(buffers[UNIFORM_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffers[UNIFORM_BUFFER].size),
                    };
                    // FULL FLUSH!!!!!!!
                    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZES(bar), bar, 0, nullptr);
                */

            }
        }




        ////////////////////////
        //
        // COMPUTES START
        //
        ////////////////////////

        beginDebugRegion(commandBuffer, "Render scene", Vec4(0.5f, 0.76f, 0.34f, 1.0f));
        insertDebugRegion(commandBuffer, "Compute", Vec4(0.0f));
        {
            bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[PIPELINE_COMPUTE_RESET]);
            vkCmdDispatch(commandBuffer, 1, 1, 1);

            VkBufferMemoryBarrier bar[]
            {
                bufferBarrier(buffers[ATOMIC_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
            };
            // FULL FLUSH!!!!!!!
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZES(bar), bar, 0, nullptr);
        }


        {
            bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[PIPELINE_COMPUTE_MESHES_BUILD]);
            vkCmdDispatch(commandBuffer, drawCount, 1, 1);

            VkBufferMemoryBarrier bar[]
            {
                //bufferBarrier(buffers[ATOMIC_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffers[ATOMIC_BUFFER].size),
                bufferBarrier(buffers[INSTANCE_INDEX_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
            };
            // FULL FLUSH!!!!!!!
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZES(bar), bar, 0, nullptr);
        }

        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, queryPool, TIME_POINTS::COMPUTE_BEFORE_TRIANGLE_BUILD);

        {
            bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[PIPELINE_COMPUTE_TRIANGLE_BUILD]);
            //const u32 groupSize = trianglesPerPatch;
            //vkCmdDispatch(commandBuffer, (meshData.indiceCount + groupSize * groupSize * 3 - 1) / (groupSize * groupSize * 3), drawCount, 1);
            vkCmdDispatchIndirect(commandBuffer, buffers[ATOMIC_BUFFER].buffer, VkDeviceSize(8u * sizeof(u32)));
        }

        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, queryPool, TIME_POINTS::COMPUTE_FINISHED);


        // Prepare for transition from compute to rendering
        {
            VkImageMemoryBarrier renderBeginBarriers[] =
            {
                imageBarrier(renderTargetImages[MAIN_COLOR_TARGET].image,
                    0, VK_IMAGE_LAYOUT_UNDEFINED,
                    0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),

                imageBarrier(renderTargetImages[MAIN_DEPTH_TARGET].image,
                    0, VK_IMAGE_LAYOUT_UNDEFINED,
                    0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_ASPECT_DEPTH_BIT)
            };

            // FULL FLUSH!!!!!!!
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZES(renderBeginBarriers), renderBeginBarriers);



            VkBufferMemoryBarrier bar[]
            {
                bufferBarrier(buffers[ATOMIC_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
                bufferBarrier(buffers[INDEX_WRITE_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
                bufferBarrier(buffers[VERTEX_WRITE_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
            };

            // FULL FLUSH!!!!!!!
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZES(bar), bar, 0, nullptr);

        }



        ////////////////////////
        //
        // MAIN RENDER
        //
        ////////////////////////

        {


            VkClearValue clearValues[2] = {};
            clearValues[0].color = VkClearColorValue{ {48.0f / 255.0f, 10.0f / 255.0f, 36.0f / 255.0f, 1.0f } };
            clearValues[1].depthStencil = { 0.0f, 0 };

            VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
            passBeginInfo.renderPass = renderPass;
            passBeginInfo.framebuffer = targetFB;
            passBeginInfo.renderArea.extent.width = swapchain.width;
            passBeginInfo.renderArea.extent.height = swapchain.height;
            passBeginInfo.clearValueCount = ARRAYSIZES(clearValues);
            passBeginInfo.pClearValues = clearValues;

            vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewPort = { 0.0f, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.0f, 1.0f };
            VkRect2D scissors = { { 0, 0 }, { u32(swapchain.width), u32(swapchain.height) } };

            insertDebugRegion(commandBuffer, "Render", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
            vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissors);





            // draw calls here
            // Render
            {
                //VkDeviceSize vbOffset = 0;
                //vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &vbOffset);

                //vkCmdBindIndexBuffer(commandBuffer, buffers[INDEX_DATA_BUFFER].buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
                bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[PIPELINE_GRAPHICS_PIPELINE]);
                vkCmdBindIndexBuffer(commandBuffer, buffers[INDEX_WRITE_BUFFER].buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

                //for(u32 i = 0; i < drawCount; ++i)
                {
                    // NO INDEX BUFFER BINDED
                    //vkCmdDraw(commandBuffer, mesh.indices.size() * drawCount, 1, 0, 0);

                    // With index buffer
                    //vkCmdDrawIndexed(commandBuffer, meshData.indiceCount * drawCount, 1, 0, 0, 0);

                    //vkCmdDrawIndexedIndirect(commandBuffer, buffers[ATOMIC_BUFFER].buffer, 0, 1, sizeof(u32) * 8);

                    //vkCmdPushConstants(commandBuffer, graphicsPipeline.pipelineLayout, graphicsPipeline.pushConstantStage, 0, sizeof(MeshDraw), &draws[0]);

                    // Use instanced rendering
                    vkCmdDrawIndexed(commandBuffer, meshData.indiceCount, drawCount, 0, 0, 0);
                }

                //vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            }
            vkCmdEndRenderPass(commandBuffer);
        }

        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, queryPool, TIME_POINTS::DRAW_FINISHED);






        ///////////////////////////////
        //
        // CARP number writer
        //
        //////////////////////////////
        {

            VkImageMemoryBarrier imageBarriers[] =
            {
                imageBarrier(renderTargetImages[MAIN_COLOR_TARGET].image,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL),
            };

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, //COMPUTE_SHADER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZES(imageBarriers), imageBarriers);

            bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[PIPELINE_COMPUTE_CARP_WRITE_NUMBER]);
            vkCmdDispatch(commandBuffer, 10, 2, 1);
        }

        renderTargetImages[MAIN_COLOR_TARGET].accessMask = VK_ACCESS_SHADER_WRITE_BIT;
        renderTargetImages[MAIN_COLOR_TARGET].layout = VK_IMAGE_LAYOUT_GENERAL;

        present(renderTargetImages[MAIN_COLOR_TARGET]);

        ////////////////////////
        //
        // END PASS, COLLECT TIMINGS
        //
        ////////////////////////


        uint64_t queryResults[TIME_POINTS::NUM_TIME_POINTS];
        vkGetQueryPoolResults(device, queryPool, 0, ARRAYSIZES(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT);

        //std::this_thread::sleep_for(std::chrono::milliseconds(1));

        struct TimeValues
        {
            double timeDuration[TIME_POINTS::NUM_TIME_POINTS];
        };

        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &props);

        static TimeValues timeValues = {};
        for(u32 i = TIME_POINTS::NUM_TIME_POINTS - 1; i > 0; --i)
            timeValues.timeDuration[i] += (double(queryResults[i]) - double(queryResults[i - 1])) * props.limits.timestampPeriod * 1.0e-9f;

        gpuTime += (double(queryResults[TIME_POINTS::NUM_TIME_POINTS - 1]) - double(queryResults[0])) * props.limits.timestampPeriod * 1.0e-9f;

        ++gpuframeCount;
        if(glfwGetTime() - cpuTimeStamp >= 1.0)
        {
            double d = 1000.0 / gpuframeCount;
            double e = gpuframeCount;
            double currTime = glfwGetTime();
            double cpuTime = currTime - cpuTimeStamp;
            cpuTimeStamp += 1.0f;
            double computeTotal = (timeValues.timeDuration[COMPUTE_BEFORE_TRIANGLE_BUILD] + timeValues.timeDuration[COMPUTE_FINISHED])  * d;

            printf("Gpu: %.3fms, cpu: %.3fms, compreset: %.3fms, cullingcopy: %.3fms, compute total: %.3fms, draw: %.3fms. GpuFps:%.1f, CpuFps:%.1f\n",
                (float)(gpuTime * d), (float)(cpuTime * d),
                (float)(timeValues.timeDuration[COMPUTE_BEFORE_TRIANGLE_BUILD] * d),
                (float)(timeValues.timeDuration[COMPUTE_FINISHED] * d),
                (float)(computeTotal),
                (float)(timeValues.timeDuration[DRAW_FINISHED] * d),
                e / gpuTime, e / cpuTime);
            gpuframeCount = 0u;

            for(u32 i = 0; i < TIME_POINTS::NUM_TIME_POINTS; ++i)
                timeValues.timeDuration[i] = 0.0;

            gpuTime = 0.0;
        }
    }

    VK_CHECK(vkDeviceWaitIdle(device));

}


