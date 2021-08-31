
#include "core/general.h"
#include "core/json.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

#include "core/vulkan_app.h"

#include "core/timer.h"
#include "core/mytypes.h"
#include "core/transform.h"

#include "myvulkan/vulkandevice.h"
#include "myvulkan/vulkanhelperfuncs.h"
#include "myvulkan/vulkanresource.h"
#include "myvulkan/vulkanshader.h"
#include "myvulkan/vulkanswapchain.h"

#include "math/general_math.h"
#include "math/matrix.h"
#include "math/plane.h"
#include "math/quaternion.h"
#include "math/vector3.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <memory.h>

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

enum ShaderModuleIndexes
{
	SHADER_MODULE_RENDER_QUAD_VERT,
	SHADER_MODULE_RENDER_QUAD_FRAG,

	NUM_SHADER_MODULES
};

enum BufferIndexes
{
	QUAD_BUFFER,

	INDEX_DATA_BUFFER,

	NUM_BUFFERS
};


class VulkanDrawStuff : public VulkanApp
{
public:
	VulkanDrawStuff() { }
	virtual ~VulkanDrawStuff() override;
	//bool initApp(const std::string &fontFilename);
	virtual bool init(const char *windowStr, int screenWidth, int screenHeight) override;
	virtual void run() override;

	bool createPipelines();

public:

	VkShaderModule shaderModules [NUM_SHADER_MODULES] = { };
	Buffer buffers [NUM_BUFFERS];

	std::vector<DescriptorSet> descriptorSets [NUM_SHADER_MODULES];
	PipelineWithDescriptors pipelinesWithDescriptors [NUM_PIPELINE];

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
	VkDevice device = deviceWithQueues.device;

	for(auto &pipeline : pipelinesWithDescriptors)
	{
		destroyDescriptor(device, pipeline.descriptor);
		destroyPipeline(device, pipeline.pipeline);
	}

	for(auto &buffer : buffers)
		destroyBuffer(device, buffer);

	for(auto &shaderModule : shaderModules)
		vkDestroyShaderModule(device, shaderModule, nullptr);

}

struct RenderModel
{
	struct Vertex
	{
		Vec4 pos;
		Vec4 norm;
		Vec4 color;
	};


	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};


bool readGLTF(const char *filename, RenderModel &outModel)
{
	std::string fName = std::string(filename);
	std::vector<char> buffer;

	if(!loadBytes(fName, buffer))
		return false;

	JSONBlock bl;
	bool parseSuccess = bl.parseJSON(buffer);
	printf("parsed: %i\n", parseSuccess);
	if(!parseSuccess)
		return false;

	if(parseSuccess)
	{
//		bl.print();
	}


	struct SceneNode
	{
		std::string name;
		Quat rot;
		Vec3 trans;
		uint32_t meshIndex = ~0u;
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
	};


	std::vector<SceneNode> nodes;
	std::vector<MeshNode> meshes;
	std::vector<std::vector<uint8_t>> buffers;

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
				!attribs.getChild("TEXCOORD_0").parseUInt(node.uvIndex) ||
				!attribs.getChild("COLOR_0").parseUInt(node.colorIndex))
				return false;
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

			const JSONBlock &rotBlock = child.getChild("rotation");
			rotBlock.getChild(0).parseFloat(node.rot.v.x);
			rotBlock.getChild(1).parseFloat(node.rot.v.y);
			rotBlock.getChild(2).parseFloat(node.rot.v.z);
			rotBlock.getChild(3).parseFloat(node.rot.w);

			const JSONBlock &transBlock = child.getChild("translation");
			transBlock.getChild(0).parseFloat(node.trans.x);
			transBlock.getChild(1).parseFloat(node.trans.y);
			transBlock.getChild(2).parseFloat(node.trans.z);
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
			std::vector<uint8_t> &buffer = buffers [i];

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

			//"SCALAR" 	1
			//"VEC2" 	2
			//"VEC3" 	3
			//"VEC4" 	4
			//"MAT2" 	4
			//"MAT3" 	9
			//"MAT4" 	16

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
				bool isValidVertice = componentType == 5126 || (componentType == 5123 && normalized);
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

bool VulkanDrawStuff::init(const char *windowStr, int screenWidth, int screenHeight)
{
	if(!VulkanApp::init(windowStr, screenWidth, screenHeight))
		return false;

	glfwSetWindowUserPointer(window, this);

	RenderModel renderModel;

	bool readSuccess = readGLTF("assets/models/test_gltf.gltf", renderModel);

	printf("gltf read success: %i\n", readSuccess);
	if (!readSuccess)
		return false;

	VkDevice device = deviceWithQueues.device;

	shaderModules [SHADER_MODULE_RENDER_QUAD_VERT] = loadShader(device, "assets/shader/vulkan_new/basic3d.vert.spv");
	ASSERT(shaderModules [SHADER_MODULE_RENDER_QUAD_VERT]);

	shaderModules [SHADER_MODULE_RENDER_QUAD_FRAG] = loadShader(device, "assets/shader/vulkan_new/basic3d.frag.spv");
	ASSERT(shaderModules [SHADER_MODULE_RENDER_QUAD_FRAG]);



	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	buffers [QUAD_BUFFER] = createBuffer(device, memoryProperties, 8u * 1024u * 1024u,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Quad buffer");

	buffers [INDEX_DATA_BUFFER] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");

	// Random tag data
	//struct DemoTag { const char name[17] = "debug marker tag"; } demoTag;
	//setObjectTag(device, (uint64_t)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);


	{
		uint32_t offset = 0;
		offset = uploadToScratchbuffer(scratchBuffer, ( void * )renderModel.indices.data(), size_t(sizeof(renderModel.indices[0]) * renderModel.indices.size()), offset);
		uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
			buffers [INDEX_DATA_BUFFER], scratchBuffer, offset);

		offset = 0;
		offset = uploadToScratchbuffer(scratchBuffer, (void*)renderModel.vertices.data(), size_t(sizeof(renderModel.vertices[0]) * renderModel.vertices.size()), offset);
		uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
			buffers[QUAD_BUFFER], scratchBuffer, offset);


		indicesCount = renderModel.indices.size();
	}
	return true;
}


bool VulkanDrawStuff::createPipelines()
{
	VkDevice device = deviceWithQueues.device;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
	//recreateSwapchainData();

	PipelineWithDescriptors &pipeline = pipelinesWithDescriptors [PIPELINE_GRAPHICS_PIPELINE];

	pipeline.descriptorSet = std::vector<DescriptorSet>(
		{
			DescriptorSet { VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &renderFrameBuffer, 0u, 64u * 1024u },
			DescriptorSet { VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, true, &renderFrameBuffer, 64u * 1024u, 64u * 1024u },
			DescriptorSet { VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, true, &buffers [QUAD_BUFFER] },
		});
	VertexInput vertexInput;
	pipeline.pipeline = createGraphicsPipeline(
		device, renderPass, pipelineCache,
		shaderModules [SHADER_MODULE_RENDER_QUAD_VERT],
		shaderModules [SHADER_MODULE_RENDER_QUAD_FRAG],
		vertexInput, pipeline.descriptorSet, false,
		0u, VK_SHADER_STAGE_ALL_GRAPHICS);
	pipeline.descriptor = createDescriptor(device, pipeline.descriptorSet, pipeline.pipeline.descriptorSetLayout);

	return true;
}


void VulkanDrawStuff::run()
{
	Camera camera;

	////////////////////////
	//
	// MAIN LOOP START
	// UPDATING ENGINE STATE
	//
	////////////////////////

	double previousFrameTime = glfwGetTime();
	u32 framesSinceLastDelta = 0u;
	double deltaTime = 0.0;

	u32 gpuframeCount = 0u;
	double gpuTime = 0.0;
	double cpuTimeStamp = glfwGetTime();


	VkDevice device = deviceWithQueues.device;

	while(!glfwWindowShouldClose(window))
	{
		if(++framesSinceLastDelta > 10)
		{
			double newTime = glfwGetTime();
			deltaTime = ( newTime - previousFrameTime ) / framesSinceLastDelta;
			previousFrameTime = newTime;
			framesSinceLastDelta = 0u;
		}

		glfwPollEvents();
		MouseState mouseState = getMouseState();

		checkCameraKeypresses(deltaTime, camera);
		////////////////////////
		//
		// RENDER PASSES START
		// WRITING VALUES INTO
		// "CONSTANT BUFFEERS"
		//
		////////////////////////


		struct FrameBuffer
		{
			Matrix camMat;
			Matrix viewProj;
			Matrix mvp;
			Matrix padding;
		};
		FrameBuffer b;

		//b.camMat = createMatrixFromLookAt(camera.position, camera.position + camera.forwardDir, camera.upDir);
		b.camMat = getCameraMatrix(camera);

		camera.aspectRatioWByH = float(swapchain.width) / float(swapchain.height);
		camera.fovY = 90.0f;
		camera.zFar = 2000.0f;
		camera.zNear = 0.001f;

		b.viewProj = createPerspectiveMatrix(90.0f, camera.aspectRatioWByH, 0.01f, 2000.0f);
		//b.viewProj = perspectiveProjection(camera);
		b.mvp = transpose(b.viewProj * b.camMat);

		Transform trans;
		trans.pos = Vec3(3.0f, 3.0f, 13.0f);
		static float rotationAmount = 0.0f;

		trans.rot = getQuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), rotationAmount);
		trans.scale = Vec3(3.0f, 3.0f, 3.0f);

		Transform trans2;
		trans2.pos = Vec3(10.0f, 0.0f, 0.0f);
		rotationAmount += 1.5f * dt;

		b.padding = getModelMatrix(trans2) * getModelMatrix(trans);
		{
			Vec2 fontSize(8.0f, 12.0f);
			char tmpStr[1024];

			snprintf(tmpStr, 1024, "Camera position: (%f, %f, %f), pitch: %f, yaw:%f", camera.position.x, camera.position.y, camera.position.z, camera.pitch, camera.yaw);
			fontSystem.addText(tmpStr, Vector2(10.0f, 8.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

			snprintf(tmpStr, 1024, "Camera look: (%f, %f, %f)", camera.forwardDir.x, camera.forwardDir.y, camera.forwardDir.z);
			fontSystem.addText(tmpStr, Vector2(10.0f, 22.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			snprintf(tmpStr, 1024, "Camera up: (%f, %f, %f)", camera.upDir.x, camera.upDir.y, camera.upDir.z);
			fontSystem.addText(tmpStr, Vector2(10.0f, 34.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			snprintf(tmpStr, 1024, "Camera right: (%f, %f, %f)", camera.rightDir.x, camera.rightDir.y, camera.rightDir.z);
			fontSystem.addText(tmpStr, Vector2(10.0f, 46.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));

			Matrix m = b.camMat;

			snprintf(tmpStr, 1024, "Camera mat0: (%f, %f, %f, %f)", m._00, m._01, m._02, m._03);
			fontSystem.addText(tmpStr, Vector2(10.0f, 60.0f + 12.0f * 0.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			snprintf(tmpStr, 1024, "Camera mat1: (%f, %f, %f, %f)", m._10, m._11, m._12, m._13);
			fontSystem.addText(tmpStr, Vector2(10.0f, 60.0f + 12.0f * 1.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			snprintf(tmpStr, 1024, "Camera mat2: (%f, %f, %f, %f)", m._20, m._21, m._22, m._23);
			fontSystem.addText(tmpStr, Vector2(10.0f, 60.0f + 12.0f * 2.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			snprintf(tmpStr, 1024, "Camera mat3: (%f, %f, %f, %f)", m._30, m._31, m._32, m._33);
			fontSystem.addText(tmpStr, Vector2(10.0f, 60.0f + 12.0f * 3.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));


			Vec4 p(1.0f, 1.0f, 1.0f, 1.0f);
			Vec4 p1 = mul(p, b.mvp);
			Vec4 p2 = mul(b.mvp, p);
			snprintf(tmpStr, 1024, "p1: (%f, %f, %f, %f)", p1.x, p1.y, p1.z, p1.w);
			fontSystem.addText(tmpStr, Vector2(10.0f, 110.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			snprintf(tmpStr, 1024, "p2: (%f, %f, %f, %f)", p2.x, p2.y, p2.z, p2.w);
			fontSystem.addText(tmpStr, Vector2(10.0f, 122.0f), fontSize, Vector4(1.0f, 1.0f, 1.0f, 1.0f));


		}

		if(!startRender())
			continue;

		beginSingleTimeCommands(device, commandPool, commandBuffer);
		vkCmdResetQueryPool(commandBuffer, queryPool, 0, QUERY_COUNT);
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, TIME_POINTS::START_POINT);

		{
			
			
			uint32_t offset = updateRenderFrameBuffer();
			// use scratch buffer to unifrom buffer transfer
			uint32_t bufSize = sizeof(FrameBuffer);
			memcpy(( void * )( ( char * )scratchBuffer.data + offset ), &b, bufSize);

			{
				VkBufferCopy region = { offset, 64u * 1024u, VkDeviceSize(bufSize) };
				vkCmdCopyBuffer(commandBuffer, scratchBuffer.buffer, renderFrameBuffer.buffer, 1, &region);
			}

			VkBufferMemoryBarrier bar[ ]
			{
				bufferBarrier(renderFrameBuffer.buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, bufSize),
			};

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZE(bar), bar, 0, nullptr);
		}



		////////////////////////
		//
		// MAIN RENDER
		//
		////////////////////////
		{
			VkImageMemoryBarrier imageBarriers[ ] =
			{
				imageBarrier(mainColorRenderTarget.image,
				0, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),

				imageBarrier(mainDepthRenderTarget.image,
				0, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT),
			};

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(imageBarriers), imageBarriers);
		}

		// Drawingg
		{
			VkClearValue clearValues [2] = { };
			clearValues [0].color = VkClearColorValue { { 0.0f, 0.5f, 1.0f, 1.0f } };
			clearValues [1].depthStencil = { 0.0f, 0 };

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

			insertDebugRegion(commandBuffer, "Render", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
			vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissors);


			// draw calls here
			// Render
			{
				bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinesWithDescriptors [PIPELINE_GRAPHICS_PIPELINE]);
				vkCmdBindIndexBuffer(commandBuffer, buffers [INDEX_DATA_BUFFER].buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);

			}

			fontSystem.render(commandBuffer);

			vkCmdEndRenderPass(commandBuffer);
		}

		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, queryPool, TIME_POINTS::DRAW_FINISHED);

		mainColorRenderTarget.accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		mainColorRenderTarget.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		present(mainColorRenderTarget);

		////////////////////////
		//
		// END PASS, COLLECT TIMINGS
		//
		////////////////////////


		uint64_t queryResults [TIME_POINTS::NUM_TIME_POINTS];
		vkGetQueryPoolResults(device, queryPool, 0, ARRAYSIZE(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults [0]), VK_QUERY_RESULT_64_BIT);


		struct TimeValues
		{
			double timeDuration [TIME_POINTS::NUM_TIME_POINTS];
		};

		VkPhysicalDeviceProperties props = { };
		vkGetPhysicalDeviceProperties(physicalDevice, &props);

		static TimeValues timeValues = { };
		for(u32 i = TIME_POINTS::NUM_TIME_POINTS - 1; i > 0; --i)
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

			for(u32 i = 0; i < TIME_POINTS::NUM_TIME_POINTS; ++i)
				timeValues.timeDuration [i] = 0.0;

			gpuTime = 0.0;
		}


		char str [100];
		float fps = dt > 0.0 ? float(1.0 / dt) : 0.0f;
		sprintf(str, "%2.2fms, fps: %4.2f, mx: %i, my: %i, ml: %i, mr: %i, mb: %i",
			float(dt * 1000.0), fps,
			mouseState.x, mouseState.y, mouseState.leftButtonDown, mouseState.rightButtonDown, mouseState.middleButtonDown);
		setTitle(str);


		std::this_thread::sleep_for(std::chrono::milliseconds(5));

	}

	VK_CHECK(vkDeviceWaitIdle(device));

}





int main(int argCount, char **argv)
{
	VulkanDrawStuff app;
	if(app.init("Vulkan, draw font", SCREEN_WIDTH, SCREEN_HEIGHT)
		&& app.createGraphics() && app.createPipelines())
	{
		app.run();
	}

	return 0;
}