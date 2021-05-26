#include <cstdio>
#include <cstdlib>
#include <cstdint>
// memcpy...
#include <string.h>

#include <core/timer.h>
#include <core/app.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

#include <core/vulkan_app.h>

#include <core/timer.h>
#include <core/mytypes.h>

#include <myvulkan/vulkandevice.h>
#include <myvulkan/vulkanhelperfuncs.h>
#include <myvulkan/vulkanresource.h>
#include <myvulkan/vulkanshader.h>
#include <myvulkan/vulkanswapchain.h>

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/plane.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>


static constexpr int SCREEN_WIDTH = 800;
static constexpr int SCREEN_HEIGHT = 600;


struct Entity
{
	float posX;
	float posY;
	float posZ;
	float rotation;

	float speedX;
	float speedY;
	float size;
	float padding;
};

struct GPUVertexData
{
	float posX;
	float posY;
	uint16_t pixelSizeX;
	uint16_t pixelSizeY;
	uint32_t color;

	float uvX;
	float uvY;

	float padding[ 2 ];
};

/*

struct GpuModelInstance
{
	float posX;
	float posY;
	//float posZ;
	float sinRotation;
	float cosRotation;
	//float rotation;

	uint32_t color;
	float size;
	uint32_t modelVertexStartIndex;
	uint32_t modelIndiceCount;
};
*/

struct GpuModelInstance
{
	uint32_t pos;
	uint32_t sinCosRotSize;
	uint32_t color;
	uint32_t modelVertexStartIndex;
};

struct GpuModelVertex
{
	float posX;
	float posY;
	//float posZ;

	//float padding;
};


struct Cursor
{
	float xPos = 0.0f;
	float yPos = 0.0f;

	int charWidth = 8;
	int charHeight = 12;
};

static void addText(std::string &str, std::vector<GPUVertexData> &vertData, Cursor &cursor)
{
	for (int i = 0; i < int(str.length()); ++i)
	{
		GPUVertexData vdata;
		vdata.color = core::getColor(0.0f, 1.0f, 0, 1.0f);
		vdata.pixelSizeX = cursor.charWidth;
		vdata.pixelSizeY = cursor.charHeight;
		vdata.posX = cursor.xPos;
		vdata.posY = cursor.yPos;

		uint32_t letter = str[ i ] - 32;

		vdata.uvX = float(letter) / float(128 - 32);
		vdata.uvY = 0.0f;

		vertData.emplace_back(vdata);

		cursor.xPos += cursor.charWidth;
	}

}
static void updateText(std::string &str, std::vector<GPUVertexData> &vertData, Cursor &cursor)
{
	cursor.xPos = 100.0f;
	cursor.yPos = 400.0f;
	std::string tmpStr = "w";
	tmpStr += std::to_string(cursor.charWidth);
	tmpStr += ",h";
	tmpStr += std::to_string(cursor.charHeight);
	vertData.clear();
	addText(tmpStr, vertData, cursor);

	cursor.xPos = 100.0f;
	cursor.yPos = 100.0f;
	addText(str, vertData, cursor);
}




enum TIME_POINTS
{
	START_POINT,
	DRAW_FINISHED,

	NUM_TIME_POINTS
};


// Probably not good in long run?
enum PipelineWithDescriptorsIndexes
{
	PIPELINE_GRAPHICS_PIPELINE_QUADS,
	PIPELINE_GRAPHICS_PIPELINE_MODELS,
	NUM_PIPELINE
};

enum ShaderModuleIndexes
{
	SHADER_MODULE_RENDER_QUAD_VERT,
	SHADER_MODULE_RENDER_QUAD_FRAG,

	SHADER_MODULE_RENDER_SHIP_VERT,
	SHADER_MODULE_RENDER_SHIP_FRAG,

	NUM_SHADER_MODULES
};

enum RenderTargetImageIndexes
{
	MAIN_COLOR_TARGET,
	MAIN_DEPTH_TARGET,

	NUM_TARGET_IMAGES
};

enum BufferIndexes
{
	SCRATCH_BUFFER,
	UNIFORM_BUFFER,
	QUAD_BUFFER,

	MODEL_VERTICES_BUFFER,
	INSTANCE_BUFFER,

	INDEX_DATA_BUFFER,
	INDEX_DATA_BUFFER_MODELS,

	NUM_BUFFERS
};


class VulkanTest : core::VulkanApp
{
public:
	VulkanTest() {}
	virtual ~VulkanTest() override;
	//bool initApp(const std::string &fontFilename);
	virtual bool init(const char *windowStr, int screenWidth, int screenHeight) override;
	virtual void run() override;
	virtual void recreateSwapchainData() override;
	bool initApp(const std::string &fontFilename);

	bool createGraphics();
	bool createPipelines();

public:

	VkShaderModule shaderModules[ NUM_SHADER_MODULES ] = {};
	Buffer buffers[ NUM_BUFFERS ];

	Image renderTargetImages[ NUM_TARGET_IMAGES ];

	Image textImage;
	std::vector<DescriptorSet> descriptorSets[ NUM_SHADER_MODULES ];


	PipelineWithDescriptors pipelinesWithDescriptors[ NUM_PIPELINE ];

	VkSampler textureSampler = nullptr;

	std::string fontFilename;
};



bool VulkanTest::initApp(const std::string &fontFilename)
{
	std::vector<char> data;
	if (!core::loadFontData(fontFilename, data))
	{
		printf("Failed to load file: %s\n", fontFilename.c_str());
		return false;
	}

	{
		std::vector<uint8_t> fontPic;
		fontPic.resize(( 128 - 32 ) * 8 * 12 * 4);

		// Note save order is a bit messed up!!! Since the file has one char 8x12 then next
		uint32_t index = 0;
		for (int y = 11; y >= 0; --y)
		{
			for (int charIndex = 0; charIndex < 128 - 32; ++charIndex)
			{
				uint8_t p = data[ y + size_t(charIndex) * 12 ];
				for (int x = 0; x < 8; ++x)
				{
					uint8_t bitColor = uint8_t(( p >> x ) & 1) * 255;
					fontPic[ size_t(index) * 4 + 0 ] = bitColor;
					fontPic[ size_t(index) * 4 + 1 ] = bitColor;
					fontPic[ size_t(index) * 4 + 2 ] = bitColor;
					fontPic[ size_t(index) * 4 + 3 ] = bitColor;

					++index;
				}
			}
		}
		const int textureWidth = 8 * ( 128 - 32 );
		const int textureHeight = 12;

		VkDevice device = deviceWithQueues.device;
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);


		textImage = createImage(device,
								deviceWithQueues.queueFamilyIndices.graphicsFamily, memoryProperties,
								textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_SRGB,
								VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
								"TextImage");

		updateImageWithData(device, commandBuffer, commandPool, deviceWithQueues.graphicsQueue,
							textureWidth, textureHeight, 4u,
							buffers[ SCRATCH_BUFFER ], textImage,
							( u32 ) fontPic.size(), ( void * ) fontPic.data());

		VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler));
	}

	return true;
}

////////////////////////
//
// DEINIT
//
////////////////////////

VulkanTest::~VulkanTest()
{
	VkDevice device = deviceWithQueues.device;

	for (auto &image : renderTargetImages)
		destroyImage(device, image);

	destroyImage(device, textImage);

	for (auto &pipeline : pipelinesWithDescriptors)
	{
		destroyDescriptor(device, pipeline.descriptor);
		destroyPipeline(device, pipeline.pipeline);
	}


	vkDestroySampler(device, textureSampler, nullptr);

	for (auto &buffer : buffers)
		destroyBuffer(device, buffer);

	for (auto &shaderModule : shaderModules)
		vkDestroyShaderModule(device, shaderModule, nullptr);

}

bool VulkanTest::init(const char *windowStr, int screenWidth, int screenHeight)
{
	if (!core::VulkanApp::init(windowStr, screenWidth, screenHeight))
		return false;

	glfwSetWindowUserPointer(window, this);


	VkDevice device = deviceWithQueues.device;

	shaderModules[ SHADER_MODULE_RENDER_QUAD_VERT ] = loadShader(device, "assets/shader/vulkan_new/texturedquad.vert.spv");
	ASSERT(shaderModules[ SHADER_MODULE_RENDER_QUAD_VERT ]);

	shaderModules[ SHADER_MODULE_RENDER_QUAD_FRAG ] = loadShader(device, "assets/shader/vulkan_new/texturedquad.frag.spv");
	ASSERT(shaderModules[ SHADER_MODULE_RENDER_QUAD_FRAG ]);


	shaderModules[ SHADER_MODULE_RENDER_SHIP_VERT ] = loadShader(device, "assets/shader/vulkan_new/space_ship_2d_model.vert.spv");
	ASSERT(shaderModules[ SHADER_MODULE_RENDER_SHIP_VERT ]);

	shaderModules[ SHADER_MODULE_RENDER_SHIP_FRAG ] = loadShader(device, "assets/shader/vulkan_new/space_ship_2d_model.frag.spv");
	ASSERT(shaderModules[ SHADER_MODULE_RENDER_SHIP_FRAG ]);
	

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);


	buffers[ SCRATCH_BUFFER ] = createBuffer(device, memoryProperties, 64 * 1024 * 1024,
											 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
											 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Scratch buffer");

	buffers[ UNIFORM_BUFFER ] = createBuffer(device, memoryProperties, 64u * 1024,
											 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Uniform buffer");

	buffers[ QUAD_BUFFER ] = createBuffer(device, memoryProperties, 64u * 1024u,
										  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Quad buffer");

	buffers[ MODEL_VERTICES_BUFFER ] = createBuffer(device, memoryProperties, 1024u * 1024u * 64u,
											VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Model Vercies data buffer");

	buffers[ INSTANCE_BUFFER ] = createBuffer(device, memoryProperties, 64u * 1024 * 1024u,
											  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Model Instance data buffer");



	buffers[ INDEX_DATA_BUFFER ] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
												VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
												VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");

	buffers[ INDEX_DATA_BUFFER_MODELS ] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
												VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
												VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer for models");

	

	setObjectName(device, ( uint64_t ) commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "Main command buffer");

	// Random tag data
	//struct DemoTag { const char name[17] = "debug marker tag"; } demoTag;
	//setObjectTag(device, (uint64_t)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);


	{
		uint32_t offset = 0;
		std::vector<uint32_t> indices;
		indices.resize(6 * 10240);
		for (int i = 0; i < 10240; ++i)
		{
			indices[ size_t(i) * 6 + 0 ] = i * 4 + 0;
			indices[ size_t(i) * 6 + 1 ] = i * 4 + 1;
			indices[ size_t(i) * 6 + 2 ] = i * 4 + 2;

			indices[ size_t(i) * 6 + 3 ] = i * 4 + 0;
			indices[ size_t(i) * 6 + 4 ] = i * 4 + 2;
			indices[ size_t(i) * 6 + 5 ] = i * 4 + 3;
		}
		offset = uploadToScratchbuffer(buffers[ SCRATCH_BUFFER ], ( void * ) indices.data(), size_t(sizeof(indices[ 0 ]) * indices.size()), offset);
		uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
									   buffers[ INDEX_DATA_BUFFER ], buffers[ SCRATCH_BUFFER ], offset);
	}
	return true;
}

bool VulkanTest::createGraphics()
{
	VkDevice device = deviceWithQueues.device;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
	//recreateSwapchainData();

	// create color and depth images
	{
		renderTargetImages[ MAIN_COLOR_TARGET ] =
			createImage(device, deviceWithQueues.queueFamilyIndices.graphicsFamily, memoryProperties,
						swapchain.width, swapchain.height,
						deviceWithQueues.colorFormat,
						VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
						| VK_IMAGE_USAGE_TRANSFER_SRC_BIT
						//| VK_IMAGE_USAGE_STORAGE_BIT
						, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						"Main color target image");
		renderTargetImages[ MAIN_DEPTH_TARGET ] = createImage(device, deviceWithQueues.queueFamilyIndices.graphicsFamily, memoryProperties,
															  swapchain.width, swapchain.height, deviceWithQueues.depthFormat,
															  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
															  "Main depth target image");
		targetFB = createFramebuffer(device, renderPass,
									 renderTargetImages[ MAIN_COLOR_TARGET ].imageView, renderTargetImages[ MAIN_DEPTH_TARGET ].imageView,
									 swapchain.width, swapchain.height);
	}
	return true;
}

bool VulkanTest::createPipelines()
{
	VkDevice device = deviceWithQueues.device;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	{
		PipelineWithDescriptors &pipeline = pipelinesWithDescriptors[ PIPELINE_GRAPHICS_PIPELINE_QUADS ];

		pipeline.descriptorSet = std::vector<DescriptorSet>(
			{
				DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &buffers[ UNIFORM_BUFFER ] },
				DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, true, &buffers[ QUAD_BUFFER ] },
				DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2u, true, nullptr,
				textImage.image, textImage.imageView, textureSampler, VK_IMAGE_LAYOUT_GENERAL},
			});
		VertexInput vertexInput;
		pipeline.pipeline = createGraphicsPipeline(
			device, renderPass, pipelineCache,
			shaderModules[ SHADER_MODULE_RENDER_QUAD_VERT ],
			shaderModules[ SHADER_MODULE_RENDER_QUAD_FRAG ],
			vertexInput, pipeline.descriptorSet, false,
			0u, VK_SHADER_STAGE_ALL_GRAPHICS);
		pipeline.descriptor = createDescriptor(device, pipeline.descriptorSet, pipeline.pipeline.descriptorSetLayout);
	}
	{
		PipelineWithDescriptors &pipeline = pipelinesWithDescriptors[ PIPELINE_GRAPHICS_PIPELINE_MODELS ];

		pipeline.descriptorSet = std::vector<DescriptorSet>(
			{
				DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &buffers[ UNIFORM_BUFFER ] },
				DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u, true, &buffers[ MODEL_VERTICES_BUFFER ] },
				DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3u, true, &buffers[ INSTANCE_BUFFER ] },
			});
		VertexInput vertexInput;
		pipeline.pipeline = createGraphicsPipeline(
			device, renderPass, pipelineCache,
			shaderModules[ SHADER_MODULE_RENDER_SHIP_VERT ],
			shaderModules[ SHADER_MODULE_RENDER_SHIP_FRAG ],
			vertexInput, pipeline.descriptorSet, false,
			0u, VK_SHADER_STAGE_ALL_GRAPHICS);
		pipeline.descriptor = createDescriptor(device, pipeline.descriptorSet, pipeline.pipeline.descriptorSetLayout);
	}

	return true;
}


void VulkanTest::recreateSwapchainData()
{
	VkDevice device = deviceWithQueues.device;


	vkDestroyFramebuffer(device, targetFB, nullptr);
	destroyImage(device, renderTargetImages[ MAIN_COLOR_TARGET ]);
	destroyImage(device, renderTargetImages[ MAIN_DEPTH_TARGET ]);

	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
	deviceWithQueues.queueFamilyIndices = queueFamilyIndices;
	ASSERT(deviceWithQueues.queueFamilyIndices.isValid());

	createGraphics();
	needToResize = false;
}







void VulkanTest::run()
{
	std::vector<GPUVertexData> vertData;

	std::vector < Entity > entities;
	std::vector< GpuModelInstance > modelInstances;
	std::vector< uint32_t > freeModelInstanceIndices;

	std::vector< uint32_t > modelIndices;

	std::vector < GpuModelVertex > vertices;
	constexpr uint32_t AsteroidMaxTypes = 10'000u;

	std::string txt = "abb";

	Cursor cursor;
	updateText(txt, vertData, cursor);

	{

		modelInstances.reserve(100);

	

		for (uint32_t asteroidTypes = 0u; asteroidTypes < AsteroidMaxTypes; ++asteroidTypes)
		{
			static constexpr uint32_t AsteroidCorners = 256u;

			float xPos = float(rand()) / float(RAND_MAX) * 4096.0f;
			float yPos = float(rand()) / float(RAND_MAX) * 2048.0f;
			float size = 1.0f + 1.0f * float(rand()) / float(RAND_MAX);
			entities.emplace_back(Entity{ .posX = xPos,  .posY = yPos, .posZ = 0.5f, .rotation = 0.0f, .speedX = 0.0f, .speedY = 0.0f, .size = size, .padding = 0.0f });

			/*
					modelInstances.emplace_back(GpuModelInstance{ .posX = xPos, .posY = yPos,
							//.posZ = 0.0f, .rotation = 0.0f,
							.sinRotation = 0.0f, .cosRotation = 1.0f,
							.color = core::getColor(0.5, 0.5, 0.5, 1.0f), .size = size,
						.modelVertexStartIndex = uint32_t(vertices.size()), .modelIndiceCount = AsteroidCorners });
			*/
			// Notice this quantatization doesnt work too well with high high resolutions....
			uint32_t pos = uint32_t(( xPos / 8192.0f ) * 65535.0f);
			pos += uint32_t(( yPos / 8192.0f ) * 65535.0f) << 16u;
			uint32_t sincossize = uint32_t(( size / 64.0f ) * 1023.0f);
			modelInstances.emplace_back(GpuModelInstance{ .pos = pos,
					.sinCosRotSize = sincossize,
					.color = core::getColor(0.5, 0.5, 0.5, 1.0f),
				.modelVertexStartIndex = uint32_t(vertices.size()) });


			uint32_t startIndex = uint32_t(asteroidTypes << 8u);
			vertices.emplace_back(GpuModelVertex{ .posX = 0.0f, .posY = 0.0f }); //, .posZ = 0.5f, .padding = 0.0f });
			for (uint32_t i = 0; i < AsteroidCorners; ++i)
			{
				float angle = -float(i) * float(2.0f * PI) / float(AsteroidCorners);
				float x = cos(angle);
				float y = sin(angle);
				float r = 0.8f + 0.2f * ( float(rand()) / float(RAND_MAX) );
				vertices.emplace_back(GpuModelVertex{ .posX = x * r, .posY = y * r }); //, .posZ = 0.5f, .padding = 0.0f });
				modelIndices.emplace_back(startIndex);
				modelIndices.emplace_back(( i + 1 ) % AsteroidCorners + startIndex);
				modelIndices.emplace_back(( i + 2 ) % AsteroidCorners + startIndex);
			}

			modelIndices.emplace_back(startIndex);
			modelIndices.emplace_back(AsteroidCorners - 1u + startIndex);
			modelIndices.emplace_back(1u + startIndex);
		}

		{
			float xPos = 200.0f;
			float yPos = 200.0f;
			float size = 10.0f;
			entities.emplace_back(Entity{ .posX = xPos,  .posY = yPos, .posZ = 0.5f, .rotation = 0.0f, .speedX = 0.0f, .speedY = 0.0f, .size = size, .padding = 0.0f });

			uint32_t pos = uint32_t(( xPos / 2048.0f ) * 65535.0f);
			pos += uint32_t(( yPos / 2048.0f ) * 65535.0f) << 16u;
			uint32_t sincossize = uint32_t(( size / 64.0f ) * 1023.0f);

			/*
					modelInstances.emplace_back(GpuModelInstance{ .posX = xPos, .posY = yPos,
						//.posZ = 0.0f, .rotation = 0.0f,
						.sinRotation = 0.0f, .cosRotation = 0.0f,
						.color = core::getColor(1.0f, 1.0f, 0.0f, 1.0f), .size = size,
						.modelVertexStartIndex = uint32_t(vertices.size()), .modelIndiceCount = 3 });
			*/
			modelInstances.emplace_back(GpuModelInstance{ .pos = pos,
					.sinCosRotSize = sincossize,
					.color = core::getColor(1.0, 1.0, 0.0, 1.0f),
				.modelVertexStartIndex = uint32_t(vertices.size()) });

			vertices.emplace_back(GpuModelVertex{ .posX = -1.0f, .posY = -1.0f }); //, .posZ = 0.5f, .padding = 0.0f });
			vertices.emplace_back(GpuModelVertex{ .posX = 0.0f, .posY = 1.5f });//, .posZ = 0.5f, .padding = 0.0f });
			vertices.emplace_back(GpuModelVertex{ .posX = 1.0f, .posY = -1.0f });//, .posZ = 0.5f, .padding = 0.0f });
			uint32_t startIndex = AsteroidMaxTypes << 8u;
			modelIndices.emplace_back(0 + startIndex);
			modelIndices.emplace_back(1 + startIndex);
			modelIndices.emplace_back(2 + startIndex);
		}

		u32 offset = 0u;
		offset = uploadToScratchbuffer(buffers[ SCRATCH_BUFFER ], ( void * ) modelIndices.data(), size_t(sizeof(uint32_t) * modelIndices.size()), offset);
		uploadScratchBufferToGpuBuffer(deviceWithQueues.device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
									   buffers[ INDEX_DATA_BUFFER_MODELS ], buffers[ SCRATCH_BUFFER ], offset);

	}




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
		core::MouseState mouseState = getMouseState();

		{
			Entity &playerEntity = entities[ AsteroidMaxTypes ];
			Timer updateDurTimer;
			float dtSplit = dt;
			{
				// Update position, definitely not accurate physics, if dt is big this doesn't work properly, trying to split it into several updates.
				while (dtSplit > 0.0f)
				{
					float dddt = fminf(dtSplit, 0.005f);
					float origSpeed = 1.0f * sqrtf(playerEntity.speedX * playerEntity.speedX + playerEntity.speedY * playerEntity.speedY);

					if (isDown(GLFW_KEY_LEFT) || isDown(GLFW_KEY_A))
					{
						float rotSpeed = 1.0f; // fminf(origSpeed, 1.00f);
						rotSpeed = rotSpeed * 2.0f - ( 1.0f - rotSpeed ) * 0.005;
						playerEntity.rotation += rotSpeed * dddt;
					}
					if (isDown(GLFW_KEY_RIGHT) || isDown(GLFW_KEY_D))
					{
						float rotSpeed = 1.0f; //fminf(origSpeed, 1.0f);
						rotSpeed = rotSpeed * 2.0f - ( 1.0f - rotSpeed ) * 0.005;
						playerEntity.rotation -= rotSpeed * dddt;
					}
					playerEntity.rotation = std::fmod(playerEntity.rotation, PI * 2.0);
					if (isDown(GLFW_KEY_UP) || isDown(GLFW_KEY_W))
					{
						playerEntity.speedX += cosf(playerEntity.rotation + float(PI) * 0.5f) * 5000.0f * dddt;
						playerEntity.speedY += sinf(playerEntity.rotation + float(PI) * 0.5f) * 5000.0f * dddt;
					}




					{
						float origSpeed = sqrtf(playerEntity.speedX * playerEntity.speedX + playerEntity.speedY * playerEntity.speedY);
						float dec = dddt * 0.001f * origSpeed;
						float speed = fmax(origSpeed - dec, 0.0f);
						float slowDown = 0.95f; //origSpeed > 0.01f ? speed / std::max(origSpeed, 1.0f) : 0.0f;
						playerEntity.speedX *= slowDown;
						playerEntity.speedY *= slowDown;

						playerEntity.posX += playerEntity.speedX * dddt;
						playerEntity.posY += playerEntity.speedY * dddt;
					}


					dtSplit -= dddt;
				}

				while (playerEntity.posX > windowWidth)
				{
					playerEntity.posX -= windowWidth;
				}
				while (playerEntity.posX < 0.0f)
				{
					playerEntity.posX += windowWidth;
				}
				while (playerEntity.posY > windowHeight)
				{
					playerEntity.posY -= windowHeight;
				}
				while (playerEntity.posY < 0.0f)
				{
					playerEntity.posY += windowHeight;
				}

				for (uint32_t i = 0; i < modelInstances.size(); ++i)
				{
					uint32_t pos = uint32_t(( entities[ i ].posX / 8192.0f ) * 65535.0f);
					pos += uint32_t(( entities[ i ].posY / 8192.0f ) * 65535.0f) << 16u;
					uint32_t sincossize = uint32_t(( entities[ i ].size / 64.0f ) * 1023.0f);
					float sinv = sinf(entities[ i ].rotation);
					float cosv = cosf(entities[ i ].rotation);
					sincossize += uint32_t(( sinv * 0.5f + 0.5f ) * 1023.0f) << 10u;
					sincossize += uint32_t(( cosv * 0.5f + 0.5f ) * 1023.0f) << 20u;


					modelInstances[ i ].pos = pos;
					modelInstances[ i ].sinCosRotSize = sincossize;
					/*
					modelInstances[ i ].posX = entities[ i ].posX;
					modelInstances[ i ].posY = entities[ i ].posY;
					modelInstances[ i ].sinRotation = sinf(entities[ i ].rotation);
					modelInstances[ i ].cosRotation = cosf(entities[ i ].rotation);
					//modelInstances[ i ].posZ = entities[ i ].posZ;
					//modelInstances[ i ].rotation = entities[ i ].rotation;
					*/
				}
			}
			float updateDur = float(updateDurTimer.getDuration());
		}

		ASSERT(vertData.size() * sizeof(GPUVertexData) < buffers[ QUAD_BUFFER ].size);


		////////////////////////
		//
		// RENDER PASSES START
		// WRITING VALUES INTO
		// "CONSTANT BUFFEERS"
		//
		////////////////////////

		if (!startRender())
			continue;

		beginSingleTimeCommands(device, commandPool, commandBuffer);
		vkCmdResetQueryPool(commandBuffer, queryPool, 0, QUERY_COUNT);
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, TIME_POINTS::START_POINT);

		{
			// Copy to uniform buffer
			{
				struct Buff
				{
					float windowWidth;
					float windowHeight;
					float tmp[ 6 + 8 ];
				};
				Buff buff{ float(swapchain.width), float(swapchain.height) };
				// use scratch buffer to unifrom buffer transfer
				uint32_t vertDataSize = uint32_t(vertData.size() * sizeof(GPUVertexData));
				uint32_t buffSize = uint32_t(sizeof(Buff));
				uint32_t instanceBuffserSize = uint32_t(modelInstances.size() * sizeof(GpuModelInstance));
				uint32_t modelVertexSize = uint32_t(vertices.size() * sizeof(GpuModelVertex));

				u32 memOffsets[] = { buffSize, buffSize + vertDataSize, buffSize + vertDataSize + instanceBuffserSize, modelVertexSize };

				memcpy(buffers[ SCRATCH_BUFFER ].data, &buff, buffSize);
				memcpy(( void * ) ( ( char * ) buffers[ SCRATCH_BUFFER ].data + memOffsets[0] ), vertData.data(), vertDataSize);
				memcpy(( void * ) ( ( char * ) buffers[ SCRATCH_BUFFER ].data + memOffsets[1] ), modelInstances.data(), instanceBuffserSize);
				memcpy(( void * ) ( ( char * ) buffers[ SCRATCH_BUFFER ].data + memOffsets[2] ), vertices.data(), modelVertexSize);


				{
					VkBufferCopy region = { 0, 0, VkDeviceSize(buffSize) };
					vkCmdCopyBuffer(commandBuffer, buffers[ SCRATCH_BUFFER ].buffer, buffers[ UNIFORM_BUFFER ].buffer, 1, &region);
				}
				
				{
					VkBufferCopy region = { memOffsets[0], 0, VkDeviceSize(vertDataSize) };
					vkCmdCopyBuffer(commandBuffer, buffers[ SCRATCH_BUFFER ].buffer, buffers[ QUAD_BUFFER ].buffer, 1, &region);
				}
				{
					VkBufferCopy region = { memOffsets[ 1 ], 0, VkDeviceSize(instanceBuffserSize) };
					vkCmdCopyBuffer(commandBuffer, buffers[ SCRATCH_BUFFER ].buffer, buffers[ INSTANCE_BUFFER ].buffer, 1, &region);
				}
				{
					VkBufferCopy region = { memOffsets[ 2 ], 0, VkDeviceSize(modelVertexSize) };
					vkCmdCopyBuffer(commandBuffer, buffers[ SCRATCH_BUFFER ].buffer, buffers[ MODEL_VERTICES_BUFFER ].buffer, 1, &region);
				}

				VkBufferMemoryBarrier bar[]
				{
					bufferBarrier(buffers[ UNIFORM_BUFFER ].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffSize),
					bufferBarrier(buffers[ QUAD_BUFFER ].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, vertDataSize),
					bufferBarrier(buffers[ INSTANCE_BUFFER ].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, instanceBuffserSize),
					bufferBarrier(buffers[ MODEL_VERTICES_BUFFER ].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, modelVertexSize),
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
									 VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZE(bar), bar, 0, nullptr);
			}
		}



		////////////////////////
		//
		// MAIN RENDER
		//
		////////////////////////
		{
			VkImageMemoryBarrier imageBarriers[] =
			{
				imageBarrier(renderTargetImages[ MAIN_COLOR_TARGET ].image,
							0, VK_IMAGE_LAYOUT_UNDEFINED,
							VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),

				imageBarrier(renderTargetImages[ MAIN_DEPTH_TARGET ].image,
							0, VK_IMAGE_LAYOUT_UNDEFINED,
							VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
							VK_IMAGE_ASPECT_DEPTH_BIT),
			};

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
								 VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(imageBarriers), imageBarriers);
		}

		// Drawingg
		{
			VkClearValue clearValues[ 2 ] = {};
			clearValues[ 0 ].color = VkClearColorValue{ {0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[ 1 ].depthStencil = { 0.0f, 0 };

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
				bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinesWithDescriptors[ PIPELINE_GRAPHICS_PIPELINE_MODELS ]);
				vkCmdBindIndexBuffer(commandBuffer, buffers[ INDEX_DATA_BUFFER_MODELS ].buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(commandBuffer, uint32_t(modelIndices.size()), 1, 0, 0, 0);
			
			}
			{
				bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinesWithDescriptors[ PIPELINE_GRAPHICS_PIPELINE_QUADS ]);
				vkCmdBindIndexBuffer(commandBuffer, buffers[ INDEX_DATA_BUFFER ].buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(commandBuffer, uint32_t(vertData.size() * 6), 1, 0, 0, 0);

			}
			vkCmdEndRenderPass(commandBuffer);
		}

		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, queryPool, TIME_POINTS::DRAW_FINISHED);

		renderTargetImages[ MAIN_COLOR_TARGET ].accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		renderTargetImages[ MAIN_COLOR_TARGET ].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		present(renderTargetImages[ MAIN_COLOR_TARGET ]);

		////////////////////////
		//
		// END PASS, COLLECT TIMINGS
		//
		////////////////////////


		uint64_t queryResults[ TIME_POINTS::NUM_TIME_POINTS ];
		vkGetQueryPoolResults(device, queryPool, 0, ARRAYSIZE(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[ 0 ]), VK_QUERY_RESULT_64_BIT);


		struct TimeValues
		{
			double timeDuration[ TIME_POINTS::NUM_TIME_POINTS ];
		};

		VkPhysicalDeviceProperties props = {};
		vkGetPhysicalDeviceProperties(physicalDevice, &props);

		static TimeValues timeValues = {};
		for (u32 i = TIME_POINTS::NUM_TIME_POINTS - 1; i > 0; --i)
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

			for (u32 i = 0; i < TIME_POINTS::NUM_TIME_POINTS; ++i)
				timeValues.timeDuration[ i ] = 0.0;

			gpuTime = 0.0;
		}


		std::this_thread::sleep_for(std::chrono::milliseconds(5));

	}

	VK_CHECK(vkDeviceWaitIdle(device));

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
	VulkanTest app;
	app.fontFilename = filename;
	if (app.init("Vulkan, asteroid", SCREEN_WIDTH, SCREEN_HEIGHT)
		&& app.initApp(filename) && app.createGraphics() && app.createPipelines())
	{
		app.run();
	}

	return 0;
}



