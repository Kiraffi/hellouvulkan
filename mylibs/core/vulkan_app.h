#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <core/timer.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <myvulkan/vulkandevice.h>
#include <myvulkan/vulkanswapchain.h>

constexpr u32 QUERY_COUNT = 128u;

struct Image;

namespace core
{
struct MouseState
{
	int x;
	int y;
	bool leftButtonDown;
	bool rightButtonDown;
	bool middleButtonDown;
};

struct KeyState
{
	bool isDown = false;
	uint32_t pressCount = 0u;
};

class VulkanApp
{
public:
	VulkanApp() {}
	virtual ~VulkanApp();

	virtual bool init(const char *windowStr, int screenWidth, int screenHeight);
	virtual void run() {}
	virtual void resized() {}
	virtual bool startRender();
	virtual void present(Image &presentImage);
	virtual void recreateSwapchainData() {}

	void resizeWindow(int w, int h);
	void setVsyncEnabled(bool enable);
	void setClearColor(float r, float g, float b, float a);
	void setTitle(const char *str);

	double getDeltaTime();
	MouseState getMouseState();

	bool isPressed(int keyCode);
	bool isReleased(int keyCode);
	bool isDown(int keyCode);
	bool isUp(int keyCode);

public:

	GLFWwindow *window = nullptr;
	int windowWidth = 0;
	int windowHeight = 0;
	bool vSync = true;
	bool inited = false;
	bool needToResize = false;

	KeyState keyDowns[512] = {};
	uint32_t bufferedPresses[ 128 ] = {};
	uint32_t bufferedPressesCount = 0u;

public:
	Timer timer;
	double dt = 0.0;
	uint32_t imageIndex = 0u;

	VkDebugUtilsMessengerEXT debugCallBack = 0;

	VkInstance instance = 0;
	VkSurfaceKHR surface = 0;
	VkPhysicalDevice physicalDevice = 0;

	DeviceWithQueues deviceWithQueues;

	VkRenderPass renderPass = 0;

	SwapChain swapchain;

	VkQueryPool queryPool = 0;

	VkSemaphore acquireSemaphore = 0;
	VkSemaphore releaseSemaphore = 0;

	VkFence fence = 0;

	VkCommandPool commandPool = 0;

	VkCommandBuffer commandBuffer = 0;
	VkFramebuffer targetFB = 0;


	VkPipelineCache pipelineCache = 0;
};

};