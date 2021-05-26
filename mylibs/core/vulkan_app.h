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
struct MouseState;

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

	VkCommandBuffer commandBuffer = nullptr;
	VkFramebuffer targetFB = nullptr;


	VkPipelineCache pipelineCache = nullptr;
};

};