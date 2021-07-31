#include <cstdio>
#include <cstdlib>
#include <cstdint>
// memcpy...
#include <string.h>

#include <core/timer.h>
#include <core/general.h>

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

#include "render/font_render.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>

static constexpr int SCREEN_WIDTH  = 640;
static constexpr int SCREEN_HEIGHT = 540;

struct Cursor
{
	Vector2 pos{ 0.0f, 0.0f };
	Vector2 charSize{ 8.0f, 12.0f };
};



enum TIME_POINTS
{
	START_POINT,
	DRAW_FINISHED,

	NUM_TIME_POINTS
};


enum RenderTargetImageIndexes
{
	MAIN_COLOR_TARGET,
	MAIN_DEPTH_TARGET,

	NUM_TARGET_IMAGES
};

class VulkanFontRender : core::VulkanApp
{
public:
	VulkanFontRender() {}
	virtual ~VulkanFontRender() override;
	bool initApp(const std::string &fontFilename);
	virtual bool init(const char *windowStr, int screenWidth, int screenHeight) override;
	virtual void run() override;
	virtual void recreateSwapchainData() override;

	bool createGraphics();
	void updateText(std::string& str, Cursor& cursor);
public:
	FontRenderSystem fontSystem;
	Image renderTargetImages[NUM_TARGET_IMAGES];
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanFontRender::~VulkanFontRender()
{
	VkDevice device = deviceWithQueues.device;

	fontSystem.deInit(device);

	for (auto &image : renderTargetImages)
		destroyImage(device, image);
}

bool VulkanFontRender::init(const char *windowStr, int screenWidth, int screenHeight)
{
	if (!core::VulkanApp::init(windowStr, screenWidth, screenHeight))
		return false;

	glfwSetWindowUserPointer(window, this);
	return true;
}

bool VulkanFontRender::createGraphics()
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

bool VulkanFontRender::initApp(const std::string &fontFilename)
{
	VkDevice device = deviceWithQueues.device;

	return fontSystem.init(fontFilename, device, physicalDevice, commandPool, commandBuffer, renderPass, pipelineCache,
		deviceWithQueues, scratchBuffer);

	return true;
}


void VulkanFontRender::updateText(std::string &str, Cursor &cursor)
{
	cursor.pos.x = 100.0f;
	cursor.pos.y = 400.0f;
	std::string tmpStr = "w";
	tmpStr += std::to_string(int32_t(cursor.charSize.x));
	tmpStr += ",h";
	tmpStr += std::to_string(int32_t(cursor.charSize.y));

	fontSystem.addText(tmpStr, cursor.pos, cursor.charSize);

	cursor.pos.x = 100.0f;
	cursor.pos.y = 100.0f;
	fontSystem.addText(str, cursor.pos, cursor.charSize);
}




void VulkanFontRender::recreateSwapchainData()
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







void VulkanFontRender::run()
{
	//glfwSetKeyCallback(window, key_callback);

	
	
	std::string txt = "Test";
	Cursor cursor;

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
		{
			bool textNeedsUpdate = false;
			for (int i = 0; i < bufferedPressesCount; ++i)
			{
				txt += char( bufferedPresses[i] );
				textNeedsUpdate = true;
			}

			if (keyDowns[ GLFW_KEY_LEFT ].isDown)
			{
				cursor.charSize.x--;
				if (cursor.charSize.x < 2)
					++cursor.charSize.x;
				textNeedsUpdate = true;
			}
			if (keyDowns[ GLFW_KEY_RIGHT ].isDown)
			{
				cursor.charSize.x++;
				textNeedsUpdate = true;
			}
			if (keyDowns[ GLFW_KEY_DOWN ].isDown)
			{
				cursor.charSize.y++;
				textNeedsUpdate = true;
			}
			if (keyDowns[ GLFW_KEY_UP ].isDown)
			{
				cursor.charSize.y--;
				if (cursor.charSize.y < 2)
					++cursor.charSize.y;
				textNeedsUpdate = true;
			}

			updateText(txt, cursor);
		}


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

		fontSystem.update(device, commandBuffer, renderPass, Vector2(windowWidth, windowHeight), scratchBuffer);

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


			fontSystem.render(commandBuffer);
			// draw calls here
			// Render
			{
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
	
	VulkanFontRender app;
	if(app.init("Vulkan, render font", SCREEN_WIDTH, SCREEN_HEIGHT) 
		&& app.initApp(filename) && app.createGraphics())
	{
		app.run();
	}
	
	return 0;
}