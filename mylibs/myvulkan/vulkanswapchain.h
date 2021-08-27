#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "core/mytypes.h"

// Intel?
//const VkFormat defaultFormat = VK_FORMAT_R8G8B8A8_UNORM;

#if 0 // HDR
	const VkFormat defaultFormat = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
	const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	const VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_HDR10_ST2084_EXT;
#else
	const VkFormat defaultFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM}; //, VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB };
	const VkFormat defaultDepthFormat = VK_FORMAT_D32_SFLOAT;
	const VkColorSpaceKHR defaultColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
#endif 

struct GLFWwindow;

struct SwapChain
{
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	std::vector<VkImage> images;

	u32 width = 0;
	u32 height = 0;

	u32 swapchainCount = 0;
};


struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

void destroySwapchain(SwapChain &swapchain, VkDevice device);

bool createSwapchain(SwapChain &swapChain, GLFWwindow *window, VkDevice device, VkPhysicalDevice physicalDevice, 
	VkFormat colorFormat, VkColorSpaceKHR colorSpace, VkSurfaceKHR surface);

bool resizeSwapchain(SwapChain &swapChain, GLFWwindow *window, VkDevice device, VkPhysicalDevice physicalDevice, 
	VkFormat colorFormat, VkColorSpaceKHR colorSpace, VkSurfaceKHR surface);
