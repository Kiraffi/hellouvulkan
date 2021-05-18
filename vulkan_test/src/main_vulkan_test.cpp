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

#include "timer.h"
#include "transform.h"
#include <core/mytypes.h>

#include "myvulkan/vulkandevice.h"
#include "myvulkan/vulkanresource.h"
#include "myvulkan/vulkanshader.h"
#include "myvulkan/vulkanswapchain.h"

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

const u32 QUERY_COUNT = 128u;


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

struct MainData
{
	GLFWwindow *window = nullptr;
	VkDebugUtilsMessengerEXT debugCallBack = nullptr;

	VkInstance instance = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;

	DeviceWithQueues deviceWithQueues;

	VkRenderPass renderPass = nullptr;

	SwapChain swapchain;

	VkQueryPool queryPool = nullptr;

	VkSemaphore acquireSemaphore = nullptr;
	VkSemaphore releaseSemaphore = nullptr;

	VkFence fence = nullptr;

	VkCommandPool commandPool = nullptr;

	VkShaderModule shaderModules[NUM_SHADER_MODULES] = {};

	VkCommandBuffer commandBuffer = nullptr;

	Buffer buffers[NUM_BUFFERS];

	Image renderTargetImages[NUM_TARGET_IMAGES];
	VkFramebuffer targetFB = nullptr;


	std::vector<DescriptorSet> descriptorSets[NUM_SHADER_MODULES];


	VkPipelineCache pipelineCache = nullptr;
	PipelineWithDescriptors pipelinesWithDescriptors[NUM_PIPELINE];

	VkSampler mainImageComputeWriteSampler = nullptr; // can write without sampler?



	MeshDatas meshData;
	MeshTypeData meshTypeData[NUM_MESH_TYPES];

	bool needToResize = false;
};




static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	MainData *data = reinterpret_cast<MainData *>(glfwGetWindowUserPointer(window));
	if(data) 
		data->needToResize = true;
}


VkSemaphore createSemaphore(VkDevice device)
{
	VkSemaphore semaphore = nullptr;
	VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
	return semaphore;
}

VkCommandPool createCommandPool(VkDevice device, u32 familyIndex)
{
	VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	poolCreateInfo.queueFamilyIndex = familyIndex;

	VkCommandPool commandPool = nullptr;
	VK_CHECK(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool));

	return commandPool;
}


VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
{
	VkAttachmentDescription attachments[2] = {};
	attachments[0].format = colorFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachments[1].format = depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// attachment index
	VkAttachmentReference colorAttachments = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthAttachments = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachments;
	subpass.pDepthStencilAttachment = &depthAttachments;

	VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = ARRAYSIZE(attachments);
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	VkRenderPass renderPass = nullptr;

	VK_CHECK(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass));
	return renderPass;
}



VkQueryPool createQueryPool(VkDevice device, u32 queryCount)
{
	VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };

	createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	createInfo.queryCount = queryCount;

	VkQueryPool pool = nullptr;
	VK_CHECK(vkCreateQueryPool(device, &createInfo, nullptr, &pool));

	ASSERT(pool);
	return pool;
}


VkFence createFence(VkDevice device)
{
	VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence result = nullptr;
	VK_CHECK(vkCreateFence(device, &createInfo, nullptr, &result));
	ASSERT(result);
	return result;
}







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

void deinit(MainData& mainData)
{
	VkDevice device = mainData.deviceWithQueues.device;
	for (auto& pipeline : mainData.pipelinesWithDescriptors)
	{
		destroyDescriptor(device, pipeline.descriptor);
		destroyPipeline(device, pipeline.pipeline);
	}

	vkDestroyFramebuffer(device, mainData.targetFB, nullptr);
	for (auto& image : mainData.renderTargetImages)
		destroyImage(device, image);

	for (auto& buffer : mainData.buffers)
		destroyBuffer(device, buffer);

	vkDestroyCommandPool(device, mainData.commandPool, nullptr);

	vkDestroyQueryPool(device, mainData.queryPool, nullptr);

	destroySwapchain(mainData.swapchain, device);

	vkDestroySampler(device, mainData.mainImageComputeWriteSampler, nullptr);

	for (auto& shaderModule : mainData.shaderModules)
		vkDestroyShaderModule(device, shaderModule, nullptr);


	vkDestroyRenderPass(device, mainData.renderPass, nullptr);
	vkDestroyFence(device, mainData.fence, nullptr);
	vkDestroySemaphore(device, mainData.acquireSemaphore, nullptr);
	vkDestroySemaphore(device, mainData.releaseSemaphore, nullptr);

	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(mainData.instance, mainData.surface, nullptr);

	if (enableValidationLayers)
	{
		auto dest = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mainData.instance, "vkDestroyDebugUtilsMessengerEXT");
		dest(mainData.instance, mainData.debugCallBack, nullptr);
	}
	glfwDestroyWindow(mainData.window);
	vkDestroyInstance(mainData.instance, nullptr);
}




bool init(MainData& mainData)
{
	[[maybe_unused]] int rc = glfwInit();
	ASSERT(rc);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	mainData.window = glfwCreateWindow(1024, 768, "SupaNGene2", nullptr, nullptr);
	ASSERT(mainData.window);

	glfwSetWindowUserPointer(mainData.window, &mainData);
	glfwSetFramebufferSizeCallback(mainData.window, framebufferResizeCallback);
	glfwSetKeyCallback(mainData.window, keyCallback);

	mainData.instance = createInstance();
	ASSERT(mainData.instance);


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
	mainData.meshData.indiceCount = u32(meshes[MESH_KITTEN].indices.size());

	mainData.debugCallBack = registerDebugCallback(mainData.instance);



	VK_CHECK(glfwCreateWindowSurface(mainData.instance, mainData.window, nullptr, &mainData.surface));
	ASSERT(mainData.surface);



	mainData.physicalDevice = createPhysicalDevice(mainData.instance, mainData.surface);
	ASSERT(mainData.physicalDevice);


	VkPhysicalDeviceProperties props = {};
	vkGetPhysicalDeviceProperties(mainData.physicalDevice, &props);
	ASSERT(props.limits.timestampComputeAndGraphics);

	mainData.deviceWithQueues = createDeviceWithQueues(mainData.physicalDevice, mainData.surface);
	VkDevice device = mainData.deviceWithQueues.device;
	ASSERT(device);

	{
		VkPhysicalDeviceSubgroupProperties subgroupProperties;
		subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
		subgroupProperties.pNext = NULL;

		VkPhysicalDeviceProperties2 physicalDeviceProperties;
		physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		physicalDeviceProperties.pNext = &subgroupProperties;

		vkGetPhysicalDeviceProperties2(mainData.physicalDevice, &physicalDeviceProperties);

		printf("subgroup size: %u\n", subgroupProperties.subgroupSize);
		printf("subgroup operations: %u\n", subgroupProperties.supportedOperations);
		
	}
	mainData.renderPass = createRenderPass(device, mainData.deviceWithQueues.computeColorFormat, mainData.deviceWithQueues.depthFormat);
	ASSERT(mainData.renderPass);

	[[maybe_unused]] bool scSuccess = createSwapchain(mainData.swapchain, mainData.window, device, mainData.physicalDevice, 
		mainData.deviceWithQueues.colorFormat, mainData.deviceWithQueues.colorSpace, mainData.surface, mainData.renderPass);
	ASSERT(scSuccess);

	mainData.queryPool = createQueryPool(device, QUERY_COUNT);
	ASSERT(mainData.queryPool);

	mainData.acquireSemaphore = createSemaphore(device);
	ASSERT(mainData.acquireSemaphore);

	mainData.releaseSemaphore = createSemaphore(device);
	ASSERT(mainData.releaseSemaphore);

	mainData.fence = createFence(device);
	ASSERT(mainData.fence);

	mainData.commandPool = createCommandPool(device, mainData.deviceWithQueues.queueFamilyIndices.graphicsFamily);
	ASSERT(mainData.commandPool);



	mainData.shaderModules[SHADER_MODULE_COMPUTE_RESET] = loadShader(device, "assets/shader/vulkan/computereset_comp.spv");
	ASSERT(mainData.shaderModules[SHADER_MODULE_COMPUTE_RESET]);

	mainData.shaderModules[SHADER_MODULE_COMPUTE_MESH_BUILD] = loadShader(device, "assets/shader/vulkan/compute_create_instances_comp.spv"); //compute_comp.spv");
	ASSERT(mainData.shaderModules[SHADER_MODULE_COMPUTE_MESH_BUILD]);

	mainData.shaderModules[SHADER_MODULE_COMPUTE_TRIANGLE_BUILD] = loadShader(device, "assets/shader/vulkan/compute_comp.spv"); //compute_comp.spv");
	ASSERT(mainData.shaderModules[SHADER_MODULE_COMPUTE_TRIANGLE_BUILD]);

	mainData.shaderModules[SHADER_MODULE_GRAPHICS_RENDER_VERT] = loadShader(device, "assets/shader/vulkan/triangle_vert.spv");
	ASSERT(mainData.shaderModules[SHADER_MODULE_GRAPHICS_RENDER_VERT]);

	mainData.shaderModules[SHADER_MODULE_GRAPHICS_RENDER_FRAG] = loadShader(device, "assets/shader/vulkan/triangle_frag.spv");
	ASSERT(mainData.shaderModules[SHADER_MODULE_GRAPHICS_RENDER_FRAG]);

	mainData.shaderModules[SHADER_MODULE_COMPUTE_CARP_WRITE_NUMBER] = loadShader(device, "assets/shader/vulkan/computecarpwrite_comp.spv");
	ASSERT(mainData.shaderModules[SHADER_MODULE_COMPUTE_CARP_WRITE_NUMBER]);








	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = mainData.commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &mainData.commandBuffer));


	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(mainData.physicalDevice, &memoryProperties);


	mainData.buffers[SCRATCH_BUFFER] = createBuffer(device, memoryProperties, 64 * 1024 * 1024,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Scratch buffer");

	mainData.buffers[UNIFORM_BUFFER] = createBuffer(device, memoryProperties, 64u * 1024,
		//VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer");
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Uniform buffer");


	//mainData.buffers[VERTEX_DATA_BUFFER] = createBuffer(device, memoryProperties, 128 * 1024 * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);	
	//mainData.buffers[INDEX_DATA_BUFFER] = createBuffer(device, memoryProperties, 128 * 1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	mainData.buffers[VERTEX_DATA_BUFFER] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
		/*VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |*/ VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex data buffer");
	mainData.buffers[INDEX_DATA_BUFFER] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
		/*VK_BUFFER_USAGE_INDEX_BUFFER_BIT |*/ VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");
	mainData.buffers[MESHLET_DATA_BUFFER] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Meshlet data buffer");
	#if DISCRETE_GPU
		mainData.buffers[INDEX_WRITE_BUFFER] = createBuffer(device, memoryProperties, 256 * 1024 * 1024,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index write buffer");
		mainData.buffers[VERTEX_WRITE_BUFFER] = createBuffer(device, memoryProperties, 256 * 1024 * 1024,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex write buffer");
	#else
		mainData.buffers[INDEX_WRITE_BUFFER] = createBuffer(device, memoryProperties, 128 * 1024 * 1024,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index copy buffer");
		mainData.buffers[VERTEX_WRITE_BUFFER] = createBuffer(device, memoryProperties, 128 * 1024 * 1024,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex write buffer");
	#endif
	mainData.buffers[ATOMIC_BUFFER] = createBuffer(device, memoryProperties, 1 * 64u * 1024,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Atomic buffer");


	mainData.buffers[MESH_INSTANCE_DATA_BUFFER] = createBuffer(device, memoryProperties, 1 * 64u * 1024,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Mesh instance index data buffer");
	mainData.buffers[INSTANCE_INDEX_BUFFER] = createBuffer(device, memoryProperties, 1024u * 64u * 1024u,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Instance index buffer");




	setObjectName(device, (uint64_t)mainData.commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "Main command buffer");

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
			offset = uploadToScratchbuffer(mainData.buffers[SCRATCH_BUFFER], (void*)mesh.vertices.data(), size_t(sizeof(mesh.vertices[0]) * mesh.vertices.size()), offset);
			meshTypeData[i].vertexAmount = u32(mesh.vertices.size());
		}
		uploadScratchBufferToGpuBuffer(device, mainData.commandPool, mainData.commandBuffer, mainData.deviceWithQueues.graphicsQueue,
			mainData.buffers[VERTEX_DATA_BUFFER], mainData.buffers[SCRATCH_BUFFER], offset);

		offset = 0;
		for (u32 i = 0; i < NUM_MESH_TYPES; ++i)
		{
			const Mesh& mesh = meshes[i];
			meshTypeData[i].indiceStartIndex = u32(offset);
			offset = uploadToScratchbuffer(mainData.buffers[SCRATCH_BUFFER], (void*)mesh.indices.data(), size_t(sizeof(mesh.indices[0]) * mesh.indices.size()), offset);
			meshTypeData[i].indiceAmount = u32(mesh.indices.size());
		}

		uploadScratchBufferToGpuBuffer(device, mainData.commandPool, mainData.commandBuffer, mainData.deviceWithQueues.graphicsQueue,
			mainData.buffers[INDEX_DATA_BUFFER], mainData.buffers[SCRATCH_BUFFER], offset);

		offset = 0;
		for (u32 i = 0; i < NUM_MESH_TYPES; ++i)
		{
			const Mesh& mesh = meshes[i];
			meshTypeData[i].meshletStartIndex = u32(offset);
			offset = uploadToScratchbuffer(mainData.buffers[SCRATCH_BUFFER], (void*)mesh.meshlets.data(), size_t(sizeof(mesh.meshlets[0]) * mesh.meshlets.size()), offset);
			meshTypeData[i].meshletAmount = u32(mesh.meshlets.size());
		}
		uploadScratchBufferToGpuBuffer(device, mainData.commandPool, mainData.commandBuffer, mainData.deviceWithQueues.graphicsQueue,
			mainData.buffers[MESHLET_DATA_BUFFER], mainData.buffers[SCRATCH_BUFFER], offset);
	}


	// upload mesh instance data
	{
		size_t offset = 0;
		std::vector<u32> meshTypeIndexes(drawCount);
		for (u32 i = 0; i < drawCount; ++i)
		{
			meshTypeIndexes[i] = u32(rand()) % NUM_MESH_TYPES;
		}

		offset = uploadToScratchbuffer(mainData.buffers[SCRATCH_BUFFER], (void*)meshTypeData, size_t(sizeof(meshTypeData)), 0);
		offset = uploadToScratchbuffer(mainData.buffers[SCRATCH_BUFFER], (void*)meshTypeIndexes.data(), u32(meshTypeIndexes.size() * sizeof(u32)), offset);

		uploadScratchBufferToGpuBuffer(device, mainData.commandPool, mainData.commandBuffer, mainData.deviceWithQueues.graphicsQueue,
			mainData.buffers[MESH_INSTANCE_DATA_BUFFER], mainData.buffers[SCRATCH_BUFFER], offset);
	}



	// create color and depth images
	{
		mainData.renderTargetImages[MAIN_COLOR_TARGET] = createImage(device, mainData.deviceWithQueues.queueFamilyIndices.graphicsFamily, memoryProperties,
			mainData.swapchain.width, mainData.swapchain.height, mainData.deviceWithQueues.computeColorFormat,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			"Main color target image");
		mainData.renderTargetImages[MAIN_DEPTH_TARGET] = createImage(device, mainData.deviceWithQueues.queueFamilyIndices.graphicsFamily, memoryProperties,
			mainData.swapchain.width, mainData.swapchain.height, mainData.deviceWithQueues.depthFormat,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			"Main depth target image");
		mainData.targetFB = createFramebuffer(device, mainData.renderPass,
			mainData.renderTargetImages[MAIN_COLOR_TARGET].imageView, mainData.renderTargetImages[MAIN_DEPTH_TARGET].imageView,
			mainData.swapchain.width, mainData.swapchain.height);
	}






	//VkDescriptorUpdateTemplate meshUpdateTemplate = createUpdateTemplate(device, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout);
	//ASSERT(meshUpdateTemplate);

	struct HelperStruct
	{
		static void setGraphicsPipeline(VkDevice device, PipelineWithDescriptors &pipelineWithDescriptors, MainData &mainData,
			ShaderModuleIndexes vertShaderIndex, ShaderModuleIndexes fragShaderIndex, const VertexInput &vertexInput,
			u32 pushConstantSize = 0u, VkShaderStageFlagBits pushConstantStageUsage = VK_SHADER_STAGE_ALL_GRAPHICS)
		{
			pipelineWithDescriptors.pipeline = createGraphicsPipeline(device, mainData.renderPass, mainData.pipelineCache,
				mainData.shaderModules[vertShaderIndex], mainData.shaderModules[fragShaderIndex], vertexInput, pipelineWithDescriptors.descriptorSet,
				pushConstantSize, pushConstantStageUsage);
			pipelineWithDescriptors.descriptor = createDescriptor(device, pipelineWithDescriptors.descriptorSet, pipelineWithDescriptors.pipeline.descriptorSetLayout);
		}

		static void setComputePipeline(VkDevice device, PipelineWithDescriptors &pipelineWithDescriptors, MainData &mainData,
			ShaderModuleIndexes computeShaderIndex,
			u32 pushConstantSize = 0u, VkShaderStageFlagBits pushConstantStageUsage = VK_SHADER_STAGE_ALL_GRAPHICS)
		{
			pipelineWithDescriptors.pipeline = createComputePipeline(device, mainData.pipelineCache, mainData.shaderModules[computeShaderIndex],
				pipelineWithDescriptors.descriptorSet, pushConstantSize, pushConstantStageUsage);
			pipelineWithDescriptors.descriptor = createDescriptor(device, pipelineWithDescriptors.descriptorSet, pipelineWithDescriptors.pipeline.descriptorSetLayout);

		}
	};


	{
		PipelineWithDescriptors &pipeline = mainData.pipelinesWithDescriptors[PIPELINE_GRAPHICS_PIPELINE];
		VertexInput vertexInput;
		//vertexInput.formats.push_back(VK_FORMAT_R32G32B32_SFLOAT);
		//vertexInput.formats.push_back(VK_FORMAT_R32G32B32_SFLOAT);
		//vertexInput.formats.push_back(VK_FORMAT_R32G32_SFLOAT);

		pipeline.descriptorSet = std::vector<DescriptorSet>(
		{
			DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &mainData.buffers[UNIFORM_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, true, &mainData.buffers[VERTEX_DATA_BUFFER] },
			//DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, true, &indexCopyBuffer };
			//DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3u, true, &atomicBuffer };
			DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 8u, true, &mainData.buffers[VERTEX_WRITE_BUFFER] },
		});

		HelperStruct::setGraphicsPipeline(device, pipeline, mainData, SHADER_MODULE_GRAPHICS_RENDER_VERT, SHADER_MODULE_GRAPHICS_RENDER_FRAG, vertexInput, 
			u32(sizeof(MeshDraw)), VK_SHADER_STAGE_ALL_GRAPHICS);
	}

	{
		PipelineWithDescriptors& pipeline = mainData.pipelinesWithDescriptors[PIPELINE_COMPUTE_MESHES_BUILD];
		pipeline.descriptorSet = std::vector<DescriptorSet>(
			{
				DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &mainData.buffers[UNIFORM_BUFFER] },

				DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3u, true, &mainData.buffers[INDEX_DATA_BUFFER] },
				DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4u, true, &mainData.buffers[ATOMIC_BUFFER] },

				DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6u, true, &mainData.buffers[MESH_INSTANCE_DATA_BUFFER] },
				DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7u, true, &mainData.buffers[INSTANCE_INDEX_BUFFER] },
			});
		HelperStruct::setComputePipeline(device, pipeline, mainData, SHADER_MODULE_COMPUTE_MESH_BUILD);
		
	}
	{
		PipelineWithDescriptors &pipeline = mainData.pipelinesWithDescriptors[PIPELINE_COMPUTE_TRIANGLE_BUILD];
		pipeline.descriptorSet = std::vector<DescriptorSet>(
		{
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &mainData.buffers[UNIFORM_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, true, &mainData.buffers[VERTEX_DATA_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, true, &mainData.buffers[INDEX_WRITE_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3u, true, &mainData.buffers[INDEX_DATA_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4u, true, &mainData.buffers[ATOMIC_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5u, true, &mainData.buffers[MESHLET_DATA_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6u, true, &mainData.buffers[MESH_INSTANCE_DATA_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 7u, true, &mainData.buffers[INSTANCE_INDEX_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 8u, true, &mainData.buffers[VERTEX_WRITE_BUFFER] },
			}); 
		HelperStruct::setComputePipeline(device, pipeline, mainData, SHADER_MODULE_COMPUTE_TRIANGLE_BUILD);
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

		VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &mainData.mainImageComputeWriteSampler));
	}

	{
		PipelineWithDescriptors &pipeline = mainData.pipelinesWithDescriptors[PIPELINE_COMPUTE_CARP_WRITE_NUMBER];
		pipeline.descriptorSet = std::vector<DescriptorSet>(
		{
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &mainData.buffers[UNIFORM_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, true, &mainData.buffers[INDEX_WRITE_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4u, true, &mainData.buffers[ATOMIC_BUFFER] },
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6u, true, nullptr,
				mainData.renderTargetImages[MAIN_COLOR_TARGET].image, mainData.renderTargetImages[MAIN_COLOR_TARGET].imageView, mainData.mainImageComputeWriteSampler, VK_IMAGE_LAYOUT_GENERAL},
		});
		HelperStruct::setComputePipeline(device, pipeline, mainData, SHADER_MODULE_COMPUTE_CARP_WRITE_NUMBER); 
	}

	{
		PipelineWithDescriptors &pipeline = mainData.pipelinesWithDescriptors[PIPELINE_COMPUTE_RESET];
		pipeline.descriptorSet = std::vector<DescriptorSet>(
		{
			DescriptorSet{ VK_SHADER_STAGE_COMPUTE_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4u, true, &mainData.buffers[ATOMIC_BUFFER] },
		});
		HelperStruct::setComputePipeline(device, pipeline, mainData, SHADER_MODULE_COMPUTE_RESET); 
	}

	return true;
}




void recreateSwapchainData(MainData &mainData)
{
	VkDevice device = mainData.deviceWithQueues.device;


	vkDestroyFramebuffer(device, mainData.targetFB, nullptr);
	destroyImage(device, mainData.renderTargetImages[MAIN_COLOR_TARGET]);
	destroyImage(device, mainData.renderTargetImages[MAIN_DEPTH_TARGET]);

	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(mainData.physicalDevice, mainData.surface);
	mainData.deviceWithQueues.queueFamilyIndices = queueFamilyIndices;
	ASSERT(mainData.deviceWithQueues.queueFamilyIndices.isValid());

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(mainData.physicalDevice, &memoryProperties);

	mainData.renderTargetImages[MAIN_COLOR_TARGET] = createImage(device, queueFamilyIndices.graphicsFamily, memoryProperties, 
		mainData.swapchain.width, mainData.swapchain.height, mainData.deviceWithQueues.colorFormat,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Main color target image");
	mainData.renderTargetImages[MAIN_DEPTH_TARGET] = createImage(device, queueFamilyIndices.graphicsFamily, memoryProperties, 
		mainData.swapchain.width, mainData.swapchain.height, mainData.deviceWithQueues.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Main depth target image");
	mainData.targetFB = createFramebuffer(device, mainData.renderPass,
		mainData.renderTargetImages[MAIN_COLOR_TARGET].imageView, mainData.renderTargetImages[MAIN_DEPTH_TARGET].imageView,mainData. swapchain.width, mainData.swapchain.height);
	mainData.needToResize = false;

	// Should probably handle descriptor pool reseting instead of actually destroying it.
	{
		PipelineWithDescriptors &pipelineWithDescriptors = mainData.pipelinesWithDescriptors[PIPELINE_COMPUTE_CARP_WRITE_NUMBER];
		destroyDescriptor(device, pipelineWithDescriptors.descriptor);

		pipelineWithDescriptors.descriptorSet[3].image = mainData.renderTargetImages[MAIN_COLOR_TARGET].image;
		pipelineWithDescriptors.descriptorSet[3].imageView = mainData.renderTargetImages[MAIN_COLOR_TARGET].imageView;
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
	////////////////////////
	//
	// Initial values
	//
	////////////////////////

	MainData mainData;
	if(!init(mainData))
	{
		return 1;
	}

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
		transform.rot = glm::angleAxis(angle,uploadScratchBufferToGpuBuffer glm::normalize(rotationVector));

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


	VkDevice device = mainData.deviceWithQueues.device;

	while(!glfwWindowShouldClose(mainData.window))
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

		vkWaitForFences(device, 1, &mainData.fence, VK_TRUE, UINT64_MAX);

		u32 imageIndex = 0;
		{
			[[maybe_unused]]VkResult res = (vkAcquireNextImageKHR(device, mainData.swapchain.swapchain, UINT64_MAX, mainData.acquireSemaphore, VK_NULL_HANDLE, &imageIndex));


			if (res != VK_ERROR_OUT_OF_DATE_KHR)
			{
				VK_CHECK(res);
			}	
			else
			{
				if(resizeSwapchain(mainData.swapchain, mainData.window, device, mainData.physicalDevice, mainData.deviceWithQueues.computeColorFormat, mainData.deviceWithQueues.colorSpace, 
					mainData.surface, mainData.renderPass))
				{
					recreateSwapchainData(mainData);
					VK_CHECK(vkDeviceWaitIdle(device));

				}
				continue;
			}
		}

		VK_CHECK(vkResetCommandPool(device, mainData.commandPool, 0));





		////////////////////////
		//
		// RENDER PASSES START
		// WRITING VALUES INTO
		// "CONSTANT BUFFEERS"
		//
		////////////////////////


		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(mainData.commandBuffer, &beginInfo));

		
		vkCmdResetQueryPool(mainData.commandBuffer, mainData.queryPool, 0, QUERY_COUNT);
		vkCmdWriteTimestamp(mainData.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mainData.queryPool, TIME_POINTS::START_POINT);

		
		
		// Update uniform values and shove them into gpu
		{
			uniformValues.screenSize.x = (float)mainData.swapchain.width;
			uniformValues.screenSize.y = (float)mainData.swapchain.height;

			uniformValues.screenSize.z = (float)(frameIndex % 1024);
			uniformValues.screenSize.w = (float)(frameIndex % 1024);

			uniformValues.camMat = glm::lookAt(camera.position, camera.position + camera.forwardDir, camera.upDir);

			camera.aspectRatioWByH = float(mainData.swapchain.width) / float(mainData.swapchain.height);
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
				memcpy(mainData.buffers[SCRATCH_BUFFER].data, &uniformValues, sizeof(UniformValues));

				VkBufferCopy region = { 0, 0, VkDeviceSize(sizeof(UniformValues)) };
				vkCmdCopyBuffer(mainData.commandBuffer, mainData.buffers[SCRATCH_BUFFER].buffer, mainData.buffers[UNIFORM_BUFFER].buffer, 1, &region);

				VkBufferMemoryBarrier copyBarrier = bufferBarrier(mainData.buffers[UNIFORM_BUFFER].buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, sizeof(UniformValues));
				vkCmdPipelineBarrier(mainData.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyBarrier, 0, nullptr);


				/*
					void *data = nullptr;
					VK_CHECK(vkMapMemory(device, mainData.buffers[UNIFORM_BUFFER].deviceMemory, 0, sizeof(UniformValues), 0, &data));
					memcpy(mainData.buffers[UNIFORM_BUFFER].data, &uniformValues, sizeof(UniformValues));
					vkUnmapMemory(device, mainData.buffers[UNIFORM_BUFFER].deviceMemory);
					VkBufferMemoryBarrier bar[]
					{
						bufferBarrier(mainData.buffers[UNIFORM_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, mainData.buffers[UNIFORM_BUFFER].size),
					};
					// FULL FLUSH!!!!!!!
					vkCmdPipelineBarrier(mainData.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
						VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZE(bar), bar, 0, nullptr);
				*/

			}
		}




		////////////////////////
		//
		// COMPUTES START
		//
		////////////////////////

		beginDebugRegion(mainData.commandBuffer, "Render scene", glm::vec4(0.5f, 0.76f, 0.34f, 1.0f));
		insertDebugRegion(mainData.commandBuffer, "Compute", glm::vec4(0.0f));
		{
			bindPipelineWithDecriptors(mainData.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mainData.pipelinesWithDescriptors[PIPELINE_COMPUTE_RESET]);
			vkCmdDispatch(mainData.commandBuffer, 1, 1, 1);

			VkBufferMemoryBarrier bar[]
			{
				bufferBarrier(mainData.buffers[ATOMIC_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, mainData.buffers[ATOMIC_BUFFER].size),
			};
			// FULL FLUSH!!!!!!!
			vkCmdPipelineBarrier(mainData.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZE(bar), bar, 0, nullptr);
		}
		
		
		{
			bindPipelineWithDecriptors(mainData.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mainData.pipelinesWithDescriptors[PIPELINE_COMPUTE_MESHES_BUILD]);
			vkCmdDispatch(mainData.commandBuffer, drawCount, 1, 1);

			VkBufferMemoryBarrier bar[]
			{
				//bufferBarrier(mainData.buffers[ATOMIC_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, mainData.buffers[ATOMIC_BUFFER].size),
				bufferBarrier(mainData.buffers[INSTANCE_INDEX_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, mainData.buffers[INSTANCE_INDEX_BUFFER].size),
			};
			// FULL FLUSH!!!!!!!
			vkCmdPipelineBarrier(mainData.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZE(bar), bar, 0, nullptr);
		}
		
		vkCmdWriteTimestamp(mainData.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, mainData.queryPool, TIME_POINTS::COMPUTE_BEFORE_TRIANGLE_BUILD);
		
		{
			bindPipelineWithDecriptors(mainData.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mainData.pipelinesWithDescriptors[PIPELINE_COMPUTE_TRIANGLE_BUILD]);
			//const u32 groupSize = trianglesPerPatch;
			//vkCmdDispatch(mainData.commandBuffer, (mainData.meshData.indiceCount + groupSize * groupSize * 3 - 1) / (groupSize * groupSize * 3), drawCount, 1);
			vkCmdDispatchIndirect(mainData.commandBuffer, mainData.buffers[ATOMIC_BUFFER].buffer, VkDeviceSize(8u * sizeof(u32)));
		}
		
		vkCmdWriteTimestamp(mainData.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, mainData.queryPool, TIME_POINTS::COMPUTE_FINISHED);


		// Prepare for transition from compute to rendering
		{
			VkImageMemoryBarrier renderBeginBarriers[] = 
			{
				imageBarrier(mainData.renderTargetImages[MAIN_COLOR_TARGET].image, 
					0, VK_IMAGE_LAYOUT_UNDEFINED,
					0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),

				imageBarrier(mainData.renderTargetImages[MAIN_DEPTH_TARGET].image, 
					0, VK_IMAGE_LAYOUT_UNDEFINED,
					0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					VK_IMAGE_ASPECT_DEPTH_BIT)
			};

			// FULL FLUSH!!!!!!!
			vkCmdPipelineBarrier(mainData.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(renderBeginBarriers), renderBeginBarriers);

		

			VkBufferMemoryBarrier bar[]
			{
				bufferBarrier(mainData.buffers[ATOMIC_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
				bufferBarrier(mainData.buffers[INDEX_WRITE_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
				bufferBarrier(mainData.buffers[VERTEX_WRITE_BUFFER], VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT),
			};

			// FULL FLUSH!!!!!!!
			vkCmdPipelineBarrier(mainData.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
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
			passBeginInfo.renderPass = mainData.renderPass;
			passBeginInfo.framebuffer = mainData.targetFB;
			passBeginInfo.renderArea.extent.width = mainData.swapchain.width;
			passBeginInfo.renderArea.extent.height = mainData.swapchain.height;
			passBeginInfo.clearValueCount = ARRAYSIZE(clearValues);
			passBeginInfo.pClearValues = clearValues;


			vkCmdBeginRenderPass(mainData.commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewPort = { 0.0f, float(mainData.swapchain.height), float(mainData.swapchain.width), -float(mainData.swapchain.height), 0.0f, 1.0f };
			VkRect2D scissors = { { 0, 0 }, { u32(mainData.swapchain.width), u32(mainData.swapchain.height) } };

			insertDebugRegion(mainData.commandBuffer, "Render", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
			vkCmdSetViewport(mainData.commandBuffer, 0, 1, &viewPort);
			vkCmdSetScissor(mainData.commandBuffer, 0, 1, &scissors);





			// draw calls here
			// Render
			{
				//VkDeviceSize vbOffset = 0;
				//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &vbOffset);

				//vkCmdBindIndexBuffer(mainData.commandBuffer, mainData.buffers[INDEX_DATA_BUFFER].buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
				bindPipelineWithDecriptors(mainData.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mainData.pipelinesWithDescriptors[PIPELINE_GRAPHICS_PIPELINE]);
				vkCmdBindIndexBuffer(mainData.commandBuffer, mainData.buffers[INDEX_WRITE_BUFFER].buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

				//for(u32 i = 0; i < drawCount; ++i)
				{
					// NO INDEX BUFFER BINDED
					//vkCmdDraw(commandBuffer, mesh.indices.size() * drawCount, 1, 0, 0);

					// With index buffer
					//vkCmdDrawIndexed(mainData.commandBuffer, mainData.meshData.indiceCount * drawCount, 1, 0, 0, 0);

					vkCmdDrawIndexedIndirect(mainData.commandBuffer, mainData.buffers[ATOMIC_BUFFER].buffer, 0, 1, sizeof(u32) * 8);

					//vkCmdPushConstants(commandBuffer, graphicsPipeline.pipelineLayout, graphicsPipeline.pushConstantStage, 0, sizeof(MeshDraw), &draws[0]);

					// Use instanced rendering
					//vkCmdDrawIndexed(mainData.commandBuffer, mainData.meshData.indiceCount, drawCount, 0, 0, 0);
				}

				//vkCmdDraw(commandBuffer, 3, 1, 0, 0);

			}
			vkCmdEndRenderPass(mainData.commandBuffer);
		}
		
		vkCmdWriteTimestamp(mainData.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, mainData.queryPool, TIME_POINTS::DRAW_FINISHED);




		

		///////////////////////////////
		//
		// CARP number writer
		//
		//////////////////////////////
		{
			
			VkImageMemoryBarrier imageBarriers[] = 
			{
				imageBarrier(mainData.renderTargetImages[MAIN_COLOR_TARGET].image, 
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL),
			};

			vkCmdPipelineBarrier(mainData.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, //COMPUTE_SHADER_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(imageBarriers), imageBarriers);

			bindPipelineWithDecriptors(mainData.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mainData.pipelinesWithDescriptors[PIPELINE_COMPUTE_CARP_WRITE_NUMBER]);
			vkCmdDispatch(mainData.commandBuffer, 10, 2, 1);
		}
		



		// Copy final image to swap chain target
		{
			VkImageMemoryBarrier copyBeginBarriers[] =
			{
				imageBarrier(mainData.renderTargetImages[MAIN_COLOR_TARGET].image,
					//			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
								VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL,
								VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),

							imageBarrier(mainData.swapchain.images[imageIndex],
								0, VK_IMAGE_LAYOUT_UNDEFINED,
								VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			};

			vkCmdPipelineBarrier(mainData.commandBuffer,  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(copyBeginBarriers), copyBeginBarriers);


			insertDebugRegion(mainData.commandBuffer, "Copy to swapchain", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
/*
			VkImageCopy imageCopyRegion = {};

			imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.srcSubresource.layerCount = 1;
			imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.dstSubresource.layerCount = 1;
			imageCopyRegion.extent = { mainData.swapchain.width, mainData.swapchain.height, 1 };




			vkCmdCopyImage(mainData.commandBuffer, mainData.renderTargetImages[MAIN_COLOR_TARGET].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				mainData.swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
*/
			VkImageBlit imageBlitRegion = {};

			imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlitRegion.srcSubresource.layerCount = 1;
			imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlitRegion.dstSubresource.layerCount = 1;
			imageBlitRegion.srcOffsets[0] = VkOffset3D{0, 0, 0};
			imageBlitRegion.srcOffsets[1] = VkOffset3D{(i32)mainData.swapchain.width, (i32)mainData.swapchain.height, 1};
			imageBlitRegion.dstOffsets[0] = VkOffset3D{0, 0, 0};
			imageBlitRegion.dstOffsets[1] = VkOffset3D{(i32)mainData.swapchain.width, (i32)mainData.swapchain.height, 1};


			vkCmdBlitImage(mainData.commandBuffer, mainData.renderTargetImages[MAIN_COLOR_TARGET].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				mainData.swapchain.images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VkFilter::VK_FILTER_NEAREST);
		}

		// Prepare image for presenting.
		{
			VkImageMemoryBarrier presentBarrier = imageBarrier(mainData.swapchain.images[imageIndex],
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

			vkCmdPipelineBarrier(mainData.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &presentBarrier);
		}

		vkCmdWriteTimestamp(mainData.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mainData.queryPool, TIME_POINTS::COPY_FINISHED);

		endDebugRegion(mainData.commandBuffer);

		VK_CHECK(vkEndCommandBuffer(mainData.commandBuffer));

		// Submit
		{
			VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; //VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkResetFences(device, 1, &mainData.fence);

			VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &mainData.acquireSemaphore;
			submitInfo.pWaitDstStageMask = &submitStageMask;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &mainData.commandBuffer;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &mainData.releaseSemaphore;
			VK_CHECK(vkQueueSubmit(mainData.deviceWithQueues.graphicsQueue, 1, &submitInfo, mainData.fence));

			VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &mainData.releaseSemaphore;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &mainData.swapchain.swapchain;
			presentInfo.pImageIndices = &imageIndex;

			VkResult res = (vkQueuePresentKHR(mainData.deviceWithQueues.presentQueue, &presentInfo));
			if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
			{
				mainData.needToResize = true;
				if (resizeSwapchain(mainData.swapchain, mainData.window, device, mainData.physicalDevice, mainData.deviceWithQueues.computeColorFormat, mainData.deviceWithQueues.colorSpace, 
					mainData.surface, mainData.renderPass))
				{
					recreateSwapchainData(mainData);
				}
				mainData.needToResize = false;
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
		vkGetQueryPoolResults(device, mainData.queryPool, 0, ARRAYSIZE(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT);

		//std::this_thread::sleep_for(std::chrono::milliseconds(1));

		struct TimeValues
		{
			double timeDuration[TIME_POINTS::NUM_TIME_POINTS];
		};

		VkPhysicalDeviceProperties props = {};
		vkGetPhysicalDeviceProperties(mainData.physicalDevice, &props);

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

	deinit(mainData);
	return 0;
}


