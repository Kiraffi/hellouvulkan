#include <assert.h>
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

//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_LANG_STL11_FORCED
#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>


#include <core/vulkan_app.h>

#include <core/timer.h>
#include "transform.h"
#include <core/mytypes.h>

#include <myvulkan/vulkandevice.h>
#include <myvulkan/vulkanresource.h>
#include <myvulkan/vulkanshader.h>
#include <myvulkan/vulkanswapchain.h>

#include "model.h"
#include "camera.h"
#include "fontsystem.h"
#include "keyhandler.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <meshoptimizer/src/meshoptimizer.h>


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
enum PipelineWithDescriptorsIndexes
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
	SCRATCH_BUFFER,
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


struct PipelineWithDescriptors
{
	Pipeline pipeline; // Maybe multiple?
	Descriptor descriptor; // maybe needs more than one, possibly separated from each other?
	std::vector<DescriptorSet> descriptorSet; // maybe needs more than one set?
};




class VulkanTest : core::VulkanApp
{
public:
	VulkanTest() {}
	virtual ~VulkanTest() override;
	virtual bool init(const char *windowStr, int screenWidth, int screenHeight) override;
	virtual void run() override;
public:
	void recreateSwapchainData();

	VkShaderModule shaderModules[NUM_SHADER_MODULES] = {};
	Buffer buffers[NUM_BUFFERS];

	Image renderTargetImages[NUM_TARGET_IMAGES];
	std::vector<DescriptorSet> descriptorSets[NUM_SHADER_MODULES];


	PipelineWithDescriptors pipelinesWithDescriptors[NUM_PIPELINE];

	VkSampler mainImageComputeWriteSampler = nullptr; // can write without sampler?

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

	for (auto& pipeline : pipelinesWithDescriptors)
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
	if(!core::VulkanApp::init(windowStr, screenWidth, screenHeight))
		return false;

	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, keyCallback);


	// Threaded loads?
	Mesh meshes[NUM_MESH_TYPES];

	{
		const char* modelNames[] =
		{
			"assets/models/kitten.obj",
			"assets/models/kitten.obj",
			//"assets/models/chalet.obj"
		};

		static_assert(ARRAYSIZE(modelNames) == NUM_MESH_TYPES);
		size_t totalMeshIndices = 0;

		for (u32 i = 0; i < NUM_MESH_TYPES; ++i)
		{
			Mesh &mesh = meshes[i];
			[[maybe_unused]] bool loading = false;
			loading = loadMesh(mesh, modelNames[i], totalMeshIndices);
			ASSERT(loading);
			totalMeshIndices += (u32)mesh.indices.size();
			assert(totalMeshIndices < (1u << 20u));
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


	






	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));


	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);


	buffers[SCRATCH_BUFFER] = createBuffer(device, memoryProperties, 64 * 1024 * 1024,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Scratch buffer");

	buffers[UNIFORM_BUFFER] = createBuffer(device, memoryProperties, 64u * 1024,
		//VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer");
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




	setObjectName(device, (uint64_t)commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "Main command buffer");

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
			offset = uploadToScratchbuffer(buffers[SCRATCH_BUFFER], (void*)mesh.vertices.data(), size_t(sizeof(mesh.vertices[0]) * mesh.vertices.size()), offset);
			meshTypeData[i].vertexAmount = u32(mesh.vertices.size());
		}
		uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
			buffers[VERTEX_DATA_BUFFER], buffers[SCRATCH_BUFFER], offset);

		offset = 0;
		for (u32 i = 0; i < NUM_MESH_TYPES; ++i)
		{
			const Mesh& mesh = meshes[i];
			meshTypeData[i].indiceStartIndex = u32(offset);
			offset = uploadToScratchbuffer(buffers[SCRATCH_BUFFER], (void*)mesh.indices.data(), size_t(sizeof(mesh.indices[0]) * mesh.indices.size()), offset);
			meshTypeData[i].indiceAmount = u32(mesh.indices.size());
		}

		uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
			buffers[INDEX_DATA_BUFFER], buffers[SCRATCH_BUFFER], offset);

		offset = 0;
		for (u32 i = 0; i < NUM_MESH_TYPES; ++i)
		{
			const Mesh& mesh = meshes[i];
			meshTypeData[i].meshletStartIndex = u32(offset);
			offset = uploadToScratchbuffer(buffers[SCRATCH_BUFFER], (void*)mesh.meshlets.data(), size_t(sizeof(mesh.meshlets[0]) * mesh.meshlets.size()), offset);
			meshTypeData[i].meshletAmount = u32(mesh.meshlets.size());
		}
		uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
			buffers[MESHLET_DATA_BUFFER], buffers[SCRATCH_BUFFER], offset);
	}


	// upload mesh instance data
	{
		size_t offset = 0;
		std::vector<u32> meshTypeIndexes(drawCount);
		for (u32 i = 0; i < drawCount; ++i)
		{
			meshTypeIndexes[i] = u32(rand()) % NUM_MESH_TYPES;
		}

		offset = uploadToScratchbuffer(buffers[SCRATCH_BUFFER], (void*)meshTypeData, size_t(sizeof(meshTypeData)), 0);
		offset = uploadToScratchbuffer(buffers[SCRATCH_BUFFER], (void*)meshTypeIndexes.data(), u32(meshTypeIndexes.size() * sizeof(u32)), offset);

		uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
			buffers[MESH_INSTANCE_DATA_BUFFER], buffers[SCRATCH_BUFFER], offset);
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
		static void setGraphicsPipeline(VkDevice device, PipelineWithDescriptors &pipelineWithDescriptors, VulkanTest *test,
			ShaderModuleIndexes vertShaderIndex, ShaderModuleIndexes fragShaderIndex, const VertexInput &vertexInput,
			u32 pushConstantSize = 0u, VkShaderStageFlagBits pushConstantStageUsage = VK_SHADER_STAGE_ALL_GRAPHICS)
		{
			pipelineWithDescriptors.pipeline = createGraphicsPipeline(device, test->renderPass, test->pipelineCache,
				test->shaderModules[vertShaderIndex], test->shaderModules[fragShaderIndex], vertexInput, pipelineWithDescriptors.descriptorSet,
				pushConstantSize, pushConstantStageUsage);
			pipelineWithDescriptors.descriptor = createDescriptor(device, pipelineWithDescriptors.descriptorSet, pipelineWithDescriptors.pipeline.descriptorSetLayout);
		}

		static void setComputePipeline(VkDevice device, PipelineWithDescriptors &pipelineWithDescriptors, VulkanTest *test,
			ShaderModuleIndexes computeShaderIndex,
			u32 pushConstantSize = 0u, VkShaderStageFlagBits pushConstantStageUsage = VK_SHADER_STAGE_ALL_GRAPHICS)
		{
			pipelineWithDescriptors.pipeline = createComputePipeline(device, test->pipelineCache, test->shaderModules[computeShaderIndex],
				pipelineWithDescriptors.descriptorSet, pushConstantSize, pushConstantStageUsage);
			pipelineWithDescriptors.descriptor = createDescriptor(device, pipelineWithDescriptors.descriptorSet, pipelineWithDescriptors.pipeline.descriptorSetLayout);

		}
	};


	{
		PipelineWithDescriptors &pipeline = pipelinesWithDescriptors[PIPELINE_GRAPHICS_PIPELINE];
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
		PipelineWithDescriptors& pipeline = pipelinesWithDescriptors[PIPELINE_COMPUTE_MESHES_BUILD];
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
		PipelineWithDescriptors &pipeline = pipelinesWithDescriptors[PIPELINE_COMPUTE_TRIANGLE_BUILD];
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
		PipelineWithDescriptors &pipeline = pipelinesWithDescriptors[PIPELINE_COMPUTE_CARP_WRITE_NUMBER];
		pipeline.descriptorSet = std::vector<DescriptorSet>(
		{
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &buffers[UNIFORM_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, true, &buffers[INDEX_WRITE_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4u, true, &buffers[ATOMIC_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6u, true, nullptr,
				renderTargetImages[MAIN_COLOR_TARGET].image, renderTargetImages[MAIN_COLOR_TARGET].imageView, mainImageComputeWriteSampler, VK_IMAGE_LAYOUT_GENERAL},
		});
		HelperStruct::setComputePipeline(device, pipeline, this, SHADER_MODULE_COMPUTE_CARP_WRITE_NUMBER); 
	}

	{
		PipelineWithDescriptors &pipeline = pipelinesWithDescriptors[PIPELINE_COMPUTE_RESET];
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
		PipelineWithDescriptors &pipelineWithDescriptors = pipelinesWithDescriptors[PIPELINE_COMPUTE_CARP_WRITE_NUMBER];
		destroyDescriptor(device, pipelineWithDescriptors.descriptor);

		pipelineWithDescriptors.descriptorSet[3].image = renderTargetImages[MAIN_COLOR_TARGET].image;
		pipelineWithDescriptors.descriptorSet[3].imageView = renderTargetImages[MAIN_COLOR_TARGET].imageView;
		pipelineWithDescriptors.descriptor = createDescriptor(device, pipelineWithDescriptors.descriptorSet, pipelineWithDescriptors.pipeline.descriptorSetLayout);
	}
}









void bindPipelineWithDecriptors(VkCommandBuffer cmdBuffer, VkPipelineBindPoint bindPoint, PipelineWithDescriptors &pipelineWithDescriptor)
{
	vkCmdBindPipeline(cmdBuffer, bindPoint, pipelineWithDescriptor.pipeline.pipelines[0]);
	vkCmdBindDescriptorSets(cmdBuffer, bindPoint, pipelineWithDescriptor.pipeline.pipelineLayout,
		0, 1, &pipelineWithDescriptor.descriptor.descriptorSet, 0, NULL);
}



struct UniformValues
{
	//float transforms[128 * 16];
	float transforms[1200 * 12] = {};
	//glm::mat4 transforms[512];
	glm::mat4 mvp;
	glm::mat4 camMat;

	glm::vec4 cameraForwardDir; // padded
	glm::vec4 cameraPosition; // padded
	glm::vec4 screenSize; // padded

	glm::vec4 cameraPerspectionAdding; // single value

	glm::vec4 frustumPlanes[6];
	u32 someValues[4];
	glm::vec4 padding2;

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

	glm::vec3 dirs[drawCount];

	static constexpr float SpreadDist = 5.0f;

	for(u32 i = 0; i < drawCount; ++i)
	{
		Transform &transform = meshTransforms[i];
		transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
		transform.pos = glm::vec3(i, i, 15.0f - i * 3);
		transform.pos.x = rand() / float(RAND_MAX) * 2.0f * SpreadDist - SpreadDist;
		transform.pos.y = rand() / float(RAND_MAX) * 2.0f * SpreadDist - SpreadDist;
		transform.pos.z = rand() / float(RAND_MAX) * 2.0f * SpreadDist + SpreadDist;

		glm::vec3 rotationVector;
		rotationVector.x = (rand() / float(RAND_MAX) * 2.0f - 1.0f);
		rotationVector.y = (rand() / float(RAND_MAX) * 2.0f - 1.0f);
		rotationVector.z = (rand() / float(RAND_MAX) * 2.0f - 1.0f);
		float angle = rand() / float(RAND_MAX) * pii * 2.0f;
		transform.rot = glm::angleAxis(angle, glm::normalize(rotationVector));

		//glm::mat4 modelMatrix = glm::transpose(getModelMatrix(transform));
		//uniformValues.transforms[i] = modelMatrix;

		glm::mat4 modelMatrix = glm::transpose(getModelMatrix(transform));
		memcpy(&uniformValues.transforms[i * 12], (float*)(&modelMatrix), sizeof(float) * 12);

		float newangle = rand() / float(RAND_MAX) * pii * 2.0f;

		glm::vec3 dir;
		dir.x = rand() / float(RAND_MAX) * 2.0f - 1.0f;
		dir.y = rand() / float(RAND_MAX) * 2.0f - 1.0f;
		dir.z = rand() / float(RAND_MAX) * 2.0f - 1.0f;

		dir = glm::normalize(dir);

		glm::quat rotation = glm::angleAxis(newangle, dir);
		dirs[i] = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
		dirs[i] = glm::vec3(0.0f, 0.0f, 0.0f);
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
		checkKeypresses(deltaTime, camera);

		vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

		u32 imageIndex = 0;
		{
			[[maybe_unused]]VkResult res = (vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &imageIndex));


			if (res != VK_ERROR_OUT_OF_DATE_KHR)
			{
				VK_CHECK(res);
			}	
			else
			{
				if(resizeSwapchain(swapchain, window, device, physicalDevice, deviceWithQueues.computeColorFormat, deviceWithQueues.colorSpace, 
					surface, renderPass))
				{
					recreateSwapchainData();
					VK_CHECK(vkDeviceWaitIdle(device));

				}
				continue;
			}
		}

		VK_CHECK(vkResetCommandPool(device, commandPool, 0));





		////////////////////////
		//
		// RENDER PASSES START
		// WRITING VALUES INTO
		// "CONSTANT BUFFEERS"
		//
		////////////////////////


		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		
		vkCmdResetQueryPool(commandBuffer, queryPool, 0, QUERY_COUNT);
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, TIME_POINTS::START_POINT);

		
		
		// Update uniform values and shove them into gpu
		{
			uniformValues.screenSize.x = (float)swapchain.width;
			uniformValues.screenSize.y = (float)swapchain.height;

			uniformValues.screenSize.z = (float)(frameIndex % 1024);
			uniformValues.screenSize.w = (float)(frameIndex % 1024);

			uniformValues.camMat = glm::lookAt(camera.position, camera.position + camera.forwardDir, camera.upDir);

			camera.aspectRatioWByH = float(swapchain.width) / float(swapchain.height);
			camera.fovY = 100.0f;
			camera.zFar = 0.1f;

			uniformValues.mvp = perspectiveProjection(camera) * uniformValues.camMat;
			uniformValues.cameraForwardDir = glm::vec4(camera.position, 0.0f);
			uniformValues.cameraForwardDir = glm::vec4(camera.forwardDir, 0.0f);


			float fovY = glm::radians(camera.fovY);
			float f = 1.0f / tanf(fovY / 2.0f);
			float divider = glm::max(1.0e-9f, glm::min(1.0f, f));
			float cosAngle = glm::dot(glm::vec3(0.0f, 0.0f, 1.0f), glm::normalize(glm::vec3(1.0f / divider, 1.0f, 1.0f)));
			uniformValues.cameraPerspectionAdding.x = cosAngle;
			uniformValues.cameraPerspectionAdding.y = glm::acos(cosAngle);
			uniformValues.someValues[0] = drawCount;
			//printf("Cos angle: %f\n", glm::degrees(glm::acos(cosAngle)));
			for(u32 i = 0; i < drawCount; ++i)
			{
				Transform &transform = meshTransforms[i];

				transform.pos += dirs[i] * deltaTime * 10.0f;
				
				glm::mat4 modelMatrix = glm::transpose(getModelMatrix(transform));
				memcpy(&uniformValues.transforms[i * 12], (float*)(&modelMatrix), sizeof(float) * 12);

				if(transform.pos.x > 20.0f|| transform.pos.x < -20.0f)
				{
					transform.pos.x = glm::clamp(transform.pos.x, -20.0f, 20.0f);
					dirs[i].x = -dirs[i].x;
				}
				if(transform.pos.y > 20.0f|| transform.pos.y < -20.0f)
				{
					transform.pos.y = glm::clamp(transform.pos.y, -20.0f, 20.0f);
					dirs[i].y = -dirs[i].y;
				}
				if(transform.pos.z > 80.0f|| transform.pos.z < 10.0f)
				{
					transform.pos.z = glm::clamp(transform.pos.z, 10.0f, 80.0f);
					dirs[i].z = -dirs[i].z;
				}
			}


			// Copy to uniform buffer
			{
				// use scratch buffer to unifrom buffer transfer
				memcpy(buffers[SCRATCH_BUFFER].data, &uniformValues, sizeof(UniformValues));

				VkBufferCopy region = { 0, 0, VkDeviceSize(sizeof(UniformValues)) };
				vkCmdCopyBuffer(commandBuffer, buffers[SCRATCH_BUFFER].buffer, buffers[UNIFORM_BUFFER].buffer, 1, &region);

				VkBufferMemoryBarrier copyBarrier = bufferBarrier(buffers[UNIFORM_BUFFER].buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, sizeof(UniformValues));
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyBarrier, 0, nullptr);


				/*
					void *data = nullptr;
					VK_CHECK(vkMapMemory(device, buffers[UNIFORM_BUFFER].deviceMemory, 0, sizeof(UniformValues), 0, &data));
					memcpy(buffers[UNIFORM_BUFFER].data, &uniformValues, sizeof(UniformValues));
					vkUnmapMemory(device, buffers[UNIFORM_BUFFER].deviceMemory);
					VkBufferMemoryBarrier bar[]
					{
						bufferBarrier(buffers[UNIFORM_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffers[UNIFORM_BUFFER].size),
					};
					// FULL FLUSH!!!!!!!
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
						VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZE(bar), bar, 0, nullptr);
				*/

			}
		}




		////////////////////////
		//
		// COMPUTES START
		//
		////////////////////////

		beginDebugRegion(commandBuffer, "Render scene", glm::vec4(0.5f, 0.76f, 0.34f, 1.0f));
		insertDebugRegion(commandBuffer, "Compute", glm::vec4(0.0f));
		{
			bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelinesWithDescriptors[PIPELINE_COMPUTE_RESET]);
			vkCmdDispatch(commandBuffer, 1, 1, 1);

			VkBufferMemoryBarrier bar[]
			{
				bufferBarrier(buffers[ATOMIC_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffers[ATOMIC_BUFFER].size),
			};
			// FULL FLUSH!!!!!!!
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZE(bar), bar, 0, nullptr);
		}
		
		
		{
			bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelinesWithDescriptors[PIPELINE_COMPUTE_MESHES_BUILD]);
			vkCmdDispatch(commandBuffer, drawCount, 1, 1);

			VkBufferMemoryBarrier bar[]
			{
				//bufferBarrier(buffers[ATOMIC_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffers[ATOMIC_BUFFER].size),
				bufferBarrier(buffers[INSTANCE_INDEX_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffers[INSTANCE_INDEX_BUFFER].size),
			};
			// FULL FLUSH!!!!!!!
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZE(bar), bar, 0, nullptr);
		}
		
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, queryPool, TIME_POINTS::COMPUTE_BEFORE_TRIANGLE_BUILD);
		
		{
			bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelinesWithDescriptors[PIPELINE_COMPUTE_TRIANGLE_BUILD]);
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
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(renderBeginBarriers), renderBeginBarriers);

		

			VkBufferMemoryBarrier bar[]
			{
				bufferBarrier(buffers[ATOMIC_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
				bufferBarrier(buffers[INDEX_WRITE_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
				bufferBarrier(buffers[VERTEX_WRITE_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
			};

			// FULL FLUSH!!!!!!!
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZE(bar), bar, 0, nullptr);

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
			passBeginInfo.clearValueCount = ARRAYSIZE(clearValues);
			passBeginInfo.pClearValues = clearValues;


			vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewPort = { 0.0f, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.0f, 1.0f };
			VkRect2D scissors = { { 0, 0 }, { u32(swapchain.width), u32(swapchain.height) } };

			insertDebugRegion(commandBuffer, "Render", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
			vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissors);





			// draw calls here
			// Render
			{
				//VkDeviceSize vbOffset = 0;
				//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &vbOffset);

				//vkCmdBindIndexBuffer(commandBuffer, buffers[INDEX_DATA_BUFFER].buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
				bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinesWithDescriptors[PIPELINE_GRAPHICS_PIPELINE]);
				vkCmdBindIndexBuffer(commandBuffer, buffers[INDEX_WRITE_BUFFER].buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

				//for(u32 i = 0; i < drawCount; ++i)
				{
					// NO INDEX BUFFER BINDED
					//vkCmdDraw(commandBuffer, mesh.indices.size() * drawCount, 1, 0, 0);

					// With index buffer
					//vkCmdDrawIndexed(commandBuffer, meshData.indiceCount * drawCount, 1, 0, 0, 0);

					vkCmdDrawIndexedIndirect(commandBuffer, buffers[ATOMIC_BUFFER].buffer, 0, 1, sizeof(u32) * 8);

					//vkCmdPushConstants(commandBuffer, graphicsPipeline.pipelineLayout, graphicsPipeline.pushConstantStage, 0, sizeof(MeshDraw), &draws[0]);

					// Use instanced rendering
					//vkCmdDrawIndexed(commandBuffer, meshData.indiceCount, drawCount, 0, 0, 0);
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
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(imageBarriers), imageBarriers);

			bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelinesWithDescriptors[PIPELINE_COMPUTE_CARP_WRITE_NUMBER]);
			vkCmdDispatch(commandBuffer, 10, 2, 1);
		}
		



		// Copy final image to swap chain target
		{
			VkImageMemoryBarrier copyBeginBarriers[] =
			{
				imageBarrier(renderTargetImages[MAIN_COLOR_TARGET].image,
					//			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
								VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL,
								VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),

							imageBarrier(swapchain.images[imageIndex],
								0, VK_IMAGE_LAYOUT_UNDEFINED,
								VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			};

			vkCmdPipelineBarrier(commandBuffer,  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(copyBeginBarriers), copyBeginBarriers);


			insertDebugRegion(commandBuffer, "Copy to swapchain", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
/*
			VkImageCopy imageCopyRegion = {};

			imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.srcSubresource.layerCount = 1;
			imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.dstSubresource.layerCount = 1;
			imageCopyRegion.extent = { swapchain.width, swapchain.height, 1 };




			vkCmdCopyImage(commandBuffer, renderTargetImages[MAIN_COLOR_TARGET].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
*/
			VkImageBlit imageBlitRegion = {};

			imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlitRegion.srcSubresource.layerCount = 1;
			imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlitRegion.dstSubresource.layerCount = 1;
			imageBlitRegion.srcOffsets[0] = VkOffset3D{0, 0, 0};
			imageBlitRegion.srcOffsets[1] = VkOffset3D{(i32)swapchain.width, (i32)swapchain.height, 1};
			imageBlitRegion.dstOffsets[0] = VkOffset3D{0, 0, 0};
			imageBlitRegion.dstOffsets[1] = VkOffset3D{(i32)swapchain.width, (i32)swapchain.height, 1};


			vkCmdBlitImage(commandBuffer, renderTargetImages[MAIN_COLOR_TARGET].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VkFilter::VK_FILTER_NEAREST);
		}

		// Prepare image for presenting.
		{
			VkImageMemoryBarrier presentBarrier = imageBarrier(swapchain.images[imageIndex],
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &presentBarrier);
		}

		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, TIME_POINTS::COPY_FINISHED);

		endDebugRegion(commandBuffer);

		VK_CHECK(vkEndCommandBuffer(commandBuffer));

		// Submit
		{
			VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; //VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkResetFences(device, 1, &fence);

			VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &acquireSemaphore;
			submitInfo.pWaitDstStageMask = &submitStageMask;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &releaseSemaphore;
			VK_CHECK(vkQueueSubmit(deviceWithQueues.graphicsQueue, 1, &submitInfo, fence));

			VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &releaseSemaphore;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &swapchain.swapchain;
			presentInfo.pImageIndices = &imageIndex;

			VkResult res = (vkQueuePresentKHR(deviceWithQueues.presentQueue, &presentInfo));
			if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
			{
				needToResize = true;
				if (resizeSwapchain(swapchain, window, device, physicalDevice, deviceWithQueues.computeColorFormat, deviceWithQueues.colorSpace, 
					surface, renderPass))
				{
					recreateSwapchainData();
				}
				needToResize = false;
			}
			else
			{
				VK_CHECK(res);
			}
		}

		VK_CHECK(vkDeviceWaitIdle(device));


		////////////////////////
		//
		// END PASS, COLLECT TIMINGS
		//
		////////////////////////


		uint64_t queryResults[TIME_POINTS::NUM_TIME_POINTS];
		vkGetQueryPoolResults(device, queryPool, 0, ARRAYSIZE(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT);

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


