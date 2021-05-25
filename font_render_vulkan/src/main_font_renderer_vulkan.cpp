#include <cstdio>
#include <cstdlib>
#include <cstdint>
// memcpy...
#include <string.h>

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


#include <ogl/shader.h>
#include <ogl/shaderbuffer.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>

static constexpr int SCREEN_WIDTH  = 640;
static constexpr int SCREEN_HEIGHT = 540;

struct GPUVertexData
{
	float posX;
	float posY;
	uint16_t pixelSizeX;
	uint16_t pixelSizeY;
	uint32_t color;

	float uvX;
	float uvY;

	float padding[2];
};

struct Cursor
{
	float xPos = 0.0f;
	float yPos = 0.0f;

	int charWidth = 8;
	int charHeight = 12;
};



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
	UNIFORM_BUFFER2,

	INDEX_DATA_BUFFER,

	NUM_BUFFERS
};


class VulkanTest : core::VulkanApp
{
public:
	VulkanTest() {}
	virtual ~VulkanTest() override;
	bool initApp(const std::string &fontFilename);
	virtual bool init(const char *windowStr, int screenWidth, int screenHeight) override;
	virtual void run() override;
	virtual void recreateSwapchainData() override;

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
};


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



	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);


	buffers[ SCRATCH_BUFFER ] = createBuffer(device, memoryProperties, 64 * 1024 * 1024,
											   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
											   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Scratch buffer");

	buffers[ UNIFORM_BUFFER ] = createBuffer(device, memoryProperties, 64u * 1024,
											   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											   //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer");
											   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Uniform buffer");

	buffers[ UNIFORM_BUFFER2 ] = createBuffer(device, memoryProperties, 64u * 1024,
												VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
												//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
												VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Uniform buffer2");

	buffers[ INDEX_DATA_BUFFER ] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
												  VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
												  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");

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
						  //deviceWithQueues.computeColorFormat,
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
	//recreateSwapchainData();

	PipelineWithDescriptors &pipeline = pipelinesWithDescriptors[ PIPELINE_GRAPHICS_PIPELINE ];

	pipeline.descriptorSet = std::vector<DescriptorSet>(
		{
			DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &buffers[ UNIFORM_BUFFER ] },
			DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, true, &buffers[ UNIFORM_BUFFER2 ] },
			DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2u, true, nullptr,
				textImage.image, textImage.imageView, textureSampler, VK_IMAGE_LAYOUT_GENERAL},
		});
	VertexInput vertexInput;
	pipeline.pipeline = createGraphicsPipeline(
		device, renderPass, pipelineCache,
		shaderModules[ SHADER_MODULE_RENDER_QUAD_VERT ],
		shaderModules[ SHADER_MODULE_RENDER_QUAD_FRAG ],
		vertexInput, pipeline.descriptorSet,
		0u, VK_SHADER_STAGE_ALL_GRAPHICS);
	pipeline.descriptor = createDescriptor(device, pipeline.descriptorSet, pipeline.pipeline.descriptorSetLayout);

	return true;
}





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
			 (u32)fontPic.size(), ( void * ) fontPic.data());

		VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler));
	}

	return true;
}



static void addText(std::string &str, std::vector<GPUVertexData> &vertData, Cursor &cursor)
{
	for(int i = 0; i < int(str.length()); ++i)
	{
		GPUVertexData vdata;
		vdata.color = core::getColor(0.0f, 1.0f, 0.0f, 1.0f);
		vdata.pixelSizeX = cursor.charWidth;
		vdata.pixelSizeY = cursor.charHeight;
		vdata.posX = cursor.xPos;
		vdata.posY = cursor.yPos;

		uint32_t letter = str[i] - 32;

		vdata.uvX = float(letter) / float(128-32);
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

struct MyKeyStates
{
	char writeBuffer[20] = {};
	int charsWritten = 0;
	int newWindowWidth = 0;
	int newWindowHeight = 0;

	bool quit = false;
	bool resize = false;

	bool upPress = false;
	bool downPress = false;
	bool leftPress = false;
	bool rightPress = false;
};

static MyKeyStates keyStates;


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(action == GLFW_PRESS)
	{
		switch(key)
		{
			case GLFW_KEY_ESCAPE: 
				keyStates.quit = true;
				glfwSetWindowShouldClose(window, GLFW_TRUE);
				break;
			case GLFW_KEY_LEFT:
				keyStates.leftPress = true;
				break;
			case GLFW_KEY_RIGHT:
				keyStates.rightPress = true;
				break;
			case GLFW_KEY_UP:
				keyStates.upPress = true;
				break;
			case GLFW_KEY_DOWN:
				keyStates.downPress = true;
				break;

			default:
				if(key >= 32 && key < 128)
				{
					char letter = (char)key;
					if(key >= 65 && key <= 90)
					{
						int adder = 32;
						if((mods & (GLFW_MOD_SHIFT | GLFW_MOD_CAPS_LOCK)) != 0)
							adder = 0;
						letter += adder;
					}
					keyStates.writeBuffer[keyStates.charsWritten] = letter; 
					++keyStates.charsWritten;
				}
				break;
		}

	}
	if(action == GLFW_RELEASE)
	{
		switch(key)
		{
			case GLFW_KEY_LEFT:
				keyStates.leftPress = false;
				break;
			case GLFW_KEY_RIGHT:
				keyStates.rightPress = false;
				break;
			case GLFW_KEY_UP:
				keyStates.upPress = false;
				break;
			case GLFW_KEY_DOWN:
				keyStates.downPress = false;
				break;

			default:
				break;
		}

	}
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
	glfwSetKeyCallback(window, key_callback);

	std::vector<GPUVertexData> vertData;
	
	std::string txt = "Testing";
	Cursor cursor;
	updateText(txt, vertData, cursor);

	bool quit = false;


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

	while (!glfwWindowShouldClose(window) && !quit)
	{
		if (++framesSinceLastDelta > 10)
		{
			double newTime = glfwGetTime();
			deltaTime = ( newTime - previousFrameTime ) / framesSinceLastDelta;
			previousFrameTime = newTime;
			framesSinceLastDelta = 0u;
		}

		glfwPollEvents();
		{
			if (keyStates.quit)
				quit = true;

			if (quit)
				break;

			for (int i = 0; i < keyStates.charsWritten; ++i)
			{
				txt += keyStates.writeBuffer[ i ];
			}

			if (keyStates.leftPress)
			{
				cursor.charWidth--;
				if (cursor.charWidth < 2)
					++cursor.charWidth;

			}
			if (keyStates.rightPress)
			{
				cursor.charWidth++;
			}
			if (keyStates.upPress)
			{
				cursor.charHeight++;
			}
			if (keyStates.downPress)
			{
				cursor.charHeight--;
				if (cursor.charHeight < 2)
					++cursor.charHeight;
			}

			if (keyStates.charsWritten > 0 || keyStates.downPress ||
				keyStates.upPress || keyStates.leftPress || keyStates.rightPress)
				updateText(txt, vertData, cursor);

			keyStates.charsWritten = 0;
			keyStates.resize = false;
		}







		VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
		{
			[[maybe_unused]] VkResult res = ( vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &imageIndex) );


			if (res != VK_ERROR_OUT_OF_DATE_KHR)
			{
				VK_CHECK(res);
			}
			else
			{
				if (resizeSwapchain(swapchain, window, device, physicalDevice, deviceWithQueues.computeColorFormat, deviceWithQueues.colorSpace,
					surface))
				{
					recreateSwapchainData();
					VK_CHECK(vkDeviceWaitIdle(device));

				}
				continue;
			}
		}



		assert(vertData.size() < 2048);


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

		{
			// Copy to uniform buffer
			{
				struct Buff
				{
					float windowWidth;
					float windowHeight;
					float tmp[6 + 8];
				};
				Buff buff{ float(swapchain.width), float(swapchain.height) };
				// use scratch buffer to unifrom buffer transfer
				uint32_t vertDataSize = uint32_t(vertData.size() * sizeof(GPUVertexData));
				uint32_t buffSize = uint32_t(sizeof(Buff));
				memcpy(buffers[ SCRATCH_BUFFER ].data, &buff, buffSize);
				memcpy((void *)((char *)buffers[ SCRATCH_BUFFER ].data + buffSize), vertData.data(), vertDataSize);

				{
					VkBufferCopy region = { 0, 0, VkDeviceSize(buffSize) };
					vkCmdCopyBuffer(commandBuffer, buffers[ SCRATCH_BUFFER ].buffer, buffers[ UNIFORM_BUFFER ].buffer, 1, &region);
				}
				{
					VkBufferCopy region = { buffSize, 0, VkDeviceSize(vertDataSize) };
					vkCmdCopyBuffer(commandBuffer, buffers[ SCRATCH_BUFFER ].buffer, buffers[ UNIFORM_BUFFER2 ].buffer, 1, &region);
				}

				VkBufferMemoryBarrier bar[]
				{
					bufferBarrier(buffers[ UNIFORM_BUFFER ].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffSize),
					bufferBarrier(buffers[ UNIFORM_BUFFER2 ].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, vertDataSize),
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
									 VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 2, bar, 0, nullptr);
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
			clearValues[ 0 ].color = VkClearColorValue{ {48.0f / 255.0f, 10.0f / 255.0f, 36.0f / 255.0f, 1.0f } };
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
				bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinesWithDescriptors[ PIPELINE_GRAPHICS_PIPELINE ]);
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
	if(argCount < 2)
	{
		filename = "assets/font/new_font.dat";
	}
	else
	{
		filename = argv[1];
	}
	
	VulkanTest app;
	if(app.init("Vulkan, render font", SCREEN_WIDTH, SCREEN_HEIGHT) 
		&& app.initApp(filename) && app.createGraphics() && app.createPipelines())
	{
		app.run();
	}
	
	return 0;
}