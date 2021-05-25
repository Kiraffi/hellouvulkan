#include "vulkan_app.h"
#include "app.h"

#include <stdio.h>
#include <filesystem>
#include <fstream>
#include <cmath>


#include <myvulkan/vulkandevice.h>
#include <myvulkan/vulkanhelperfuncs.h>
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
	{
		data->resizeWindow(width, height);
		data->needToResize = true;
	}
}

static void keyboardHandlerCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	core::VulkanApp *data = reinterpret_cast<core::VulkanApp *>(glfwGetWindowUserPointer(window));
	if(data)
	{
		if(action == GLFW_PRESS)
		{
			if(key == GLFW_KEY_ESCAPE)
				glfwSetWindowShouldClose( window, 1 );
			else if(key >= 0 && key < 512)
				data->keyDowns[key] = true;
		}
		else if(action == GLFW_RELEASE && key >= 0 && key < 512)
		{
			data->keyDowns[key] = false;
		}
	}
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

	glfwSetKeyCallback(window, keyboardHandlerCallback);

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
		deviceWithQueues.colorFormat, deviceWithQueues.colorSpace, surface);
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

	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));
	if(!commandBuffer)
	{
		printf("Failed to create vulkan command buffer!\n");
		return false;
	}




	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);


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

void VulkanApp::present(Image &presentImage)
{

	VkDevice device = deviceWithQueues.device;
	// Copy final image to swap chain target
	{
		VkImageMemoryBarrier copyBeginBarriers[] =
		{
			imageBarrier(presentImage.image,
						presentImage.accessMask, presentImage.layout,
						VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),

					imageBarrier(swapchain.images[ imageIndex ],
						0, VK_IMAGE_LAYOUT_UNDEFINED,
						VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		};

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
								VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(copyBeginBarriers), copyBeginBarriers);


		insertDebugRegion(commandBuffer, "Copy to swapchain", Vec4(1.0f, 1.0f, 0.0f, 1.0f));

		VkImageBlit imageBlitRegion = {};

		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
		imageBlitRegion.srcOffsets[ 1 ] = VkOffset3D{ ( i32 ) swapchain.width, ( i32 ) swapchain.height, 1 };
		imageBlitRegion.dstOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
		imageBlitRegion.dstOffsets[ 1 ] = VkOffset3D{ ( i32 ) swapchain.width, ( i32 ) swapchain.height, 1 };


		vkCmdBlitImage(commandBuffer, presentImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						swapchain.images[ imageIndex ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VkFilter::VK_FILTER_NEAREST);
	}

	// Prepare image for presenting.
	{
		VkImageMemoryBarrier presentBarrier = imageBarrier(swapchain.images[ imageIndex ],
															VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
															0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
								VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &presentBarrier);
	}


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

		VkResult res = ( vkQueuePresentKHR(deviceWithQueues.presentQueue, &presentInfo) );
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
		{
			needToResize = true;
			if (resizeSwapchain(swapchain, window, device, physicalDevice, deviceWithQueues.computeColorFormat, deviceWithQueues.colorSpace,
				surface))
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
