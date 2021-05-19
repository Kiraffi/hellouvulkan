#include "vulkan_app.h"
#include "app.h"

#include <stdio.h>
#include <filesystem>
#include <fstream>
#include <cmath>


#include <myvulkan/vulkandevice.h>
#include <myvulkan/vulkanresource.h>
#include <myvulkan/vulkanshader.h>
#include <myvulkan/vulkanswapchain.h>

static double timer_frequency = 0.0;

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}




static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	core::VulkanApp *data = reinterpret_cast<core::VulkanApp *>(glfwGetWindowUserPointer(window));
	if(data) 
		data->needToResize = true;
}


static VkSemaphore createSemaphore(VkDevice device)
{
	VkSemaphore semaphore = nullptr;
	VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
	return semaphore;
}

static VkCommandPool createCommandPool(VkDevice device, u32 familyIndex)
{
	VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	poolCreateInfo.queueFamilyIndex = familyIndex;

	VkCommandPool commandPool = nullptr;
	VK_CHECK(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool));

	return commandPool;
}


static VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
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



static VkQueryPool createQueryPool(VkDevice device, u32 queryCount)
{
	VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };

	createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	createInfo.queryCount = queryCount;

	VkQueryPool pool = nullptr;
	VK_CHECK(vkCreateQueryPool(device, &createInfo, nullptr, &pool));

	ASSERT(pool);
	return pool;
}


static VkFence createFence(VkDevice device)
{
	VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence result = nullptr;
	VK_CHECK(vkCreateFence(device, &createInfo, nullptr, &result));
	ASSERT(result);
	return result;
}







namespace core {

bool VulkanApp::init(const char *windowStr, int screenWidth, int screenHeight)
{

	glfwSetErrorCallback(error_callback);
	int rc = glfwInit();
	ASSERT(rc);
	if (!rc)
	{
		printf("Couldn't initialize GLFW\n");
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	inited = true;

	window = glfwCreateWindow(screenWidth, screenHeight, windowStr, NULL, NULL);
	ASSERT(window);
	if (!window)
	{
		printf("Couldn't create glfw window\n");
		return false;
	}

	instance = createInstance();
	ASSERT(instance);
	if(!instance)
	{
		printf("Failed to create vulkan instance!\n");
		return false;
	}

	int w,h;
	glfwGetFramebufferSize(window, &w, &h);
	resizeWindow(w, h);


	debugCallBack = registerDebugCallback(instance);



	VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
	ASSERT(surface);
	if(!surface)
	{
		printf("Failed to create vulkan surface!\n");
		return false;
	}


	physicalDevice = createPhysicalDevice(instance, surface);
	ASSERT(physicalDevice);
	if(!physicalDevice)
	{
		printf("Failed to create vulkan physical device!\n");
		return false;
	}


	VkPhysicalDeviceProperties props = {};
	vkGetPhysicalDeviceProperties(physicalDevice, &props);
	ASSERT(props.limits.timestampComputeAndGraphics);
	if(!props.limits.timestampComputeAndGraphics)
	{
		printf("Physical device not supporting compute and graphics!\n");
		return false;
	}

	deviceWithQueues = createDeviceWithQueues(physicalDevice, surface);
	VkDevice device = deviceWithQueues.device;
	ASSERT(device);
	if(!device)
	{
		printf("Failed to create vulkan device!\n");
		return false;
	}

	{
		VkPhysicalDeviceSubgroupProperties subgroupProperties;
		subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
		subgroupProperties.pNext = NULL;

		VkPhysicalDeviceProperties2 physicalDeviceProperties;
		physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		physicalDeviceProperties.pNext = &subgroupProperties;

		vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);

		printf("subgroup size: %u\n", subgroupProperties.subgroupSize);
		printf("subgroup operations: %u\n", subgroupProperties.supportedOperations);
		
	}
	renderPass = createRenderPass(device, deviceWithQueues.computeColorFormat, deviceWithQueues.depthFormat);
	ASSERT(renderPass);
	if(!renderPass)
	{
		printf("Failed to create render pass!\n");
		return false;
	}

	[[maybe_unused]] bool scSuccess = createSwapchain(swapchain, window, device, physicalDevice, 
		deviceWithQueues.colorFormat, deviceWithQueues.colorSpace, surface, renderPass);
	ASSERT(scSuccess);
	if(!scSuccess)
	{
		printf("Failed to create vulkan swapchain!\n");
		return false;
	}

	queryPool = createQueryPool(device, QUERY_COUNT);
	ASSERT(queryPool);
	if(!queryPool)
	{
		printf("Failed to create vulkan query pool!\n");
		return false;
	}

	acquireSemaphore = createSemaphore(device);
	ASSERT(acquireSemaphore);
	if(!acquireSemaphore)
	{
		printf("Failed to create vulkan acquire semapohore!\n");
		return false;
	}

	releaseSemaphore = createSemaphore(device);
	ASSERT(releaseSemaphore);
	if(!releaseSemaphore)
	{
		printf("Failed to create vulkan release semaphore!\n");
		return false;
	}

	fence = createFence(device);
	ASSERT(fence);
	if(!fence)
	{
		printf("Failed to create vulkan fence!\n");
		return false;
	}

	commandPool = createCommandPool(device, deviceWithQueues.queueFamilyIndices.graphicsFamily);
	ASSERT(commandPool);
	if(!commandPool)
	{
		printf("Failed to create vulkan command pool!\n");
		return false;
	}





	// rdoc....
	//glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	//glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);

	return true;

}

VulkanApp::~VulkanApp()
{
	VkDevice device = deviceWithQueues.device;
	if(device)
	{
		vkDestroyFramebuffer(device, targetFB, nullptr);

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyQueryPool(device, queryPool, nullptr);

		destroySwapchain(swapchain, device);




		vkDestroyRenderPass(device, renderPass, nullptr);
		vkDestroyFence(device, fence, nullptr);
		vkDestroySemaphore(device, acquireSemaphore, nullptr);
		vkDestroySemaphore(device, releaseSemaphore, nullptr);

		vkDestroyDevice(device, nullptr);
	}
	vkDestroySurfaceKHR(instance, surface, nullptr);

	if (enableValidationLayers)
	{
		auto dest = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		dest(instance, debugCallBack, nullptr);
	}
	vkDestroyInstance(instance, nullptr);


	if(window)
		glfwDestroyWindow(window);
	if(inited)
		glfwTerminate();
	window = nullptr;
}

void VulkanApp::resizeWindow(int w, int h)
{
	windowWidth = w;
	windowHeight = h;
	printf("Window size: %i: %i\n", w, h);
}

void VulkanApp::setVsyncEnabled(bool enable)
{
	vSync = enable;
	// Use v-sync
}



void VulkanApp::setClearColor(float r, float g, float b, float a)
{
}

void VulkanApp::present()
{
	glfwSwapBuffers(window);
	dt = timer.getLapDuration();
}
void VulkanApp::setTitle(const char *str)
{
	glfwSetWindowTitle(window, str);
}

double VulkanApp::getDeltaTime()
{
	return dt;
}

MouseState VulkanApp::getMouseState()
{
	MouseState mouseState;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	mouseState.x = xpos;
	mouseState.y = ypos;
	mouseState.y = windowHeight - mouseState.y;

	mouseState.leftButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	mouseState.rightButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
	mouseState.middleButtonDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
	return mouseState;
}


}; // end of core namespace.
