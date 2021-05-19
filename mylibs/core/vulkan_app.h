#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <core/timer.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../myvulkan/vulkandevice.h"
#include "../myvulkan/vulkanswapchain.h"

constexpr u32 QUERY_COUNT = 128u;

namespace core
{
struct MouseState;



class VulkanApp
{
public:
	VulkanApp() {}
	virtual ~VulkanApp();

	virtual bool init(const char *windowStr, int screenWidth, int screenHeight);
	virtual void run() {}
	virtual void resized() {}
	virtual void present();

	void resizeWindow(int w, int h);
	void setVsyncEnabled(bool enable);
	void setClearColor(float r, float g, float b, float a);
	void setTitle(const char *str);

	double getDeltaTime();
	MouseState getMouseState();

public:

	GLFWwindow *window = nullptr;
	int windowWidth = 0;
	int windowHeight = 0;
	bool vSync = true;
	bool inited = false;
	bool needToResize = false;

public:
	Timer timer;
	double dt = 0.0;

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