#pragma once

#include <string_view>
#include <vulkan/vulkan_core.h>

#include "core/mytypes.h"
#include "math/vector3.h"


#if USE_RENDERDOC_MARKERS
	const bool useVulkanDebugMarkersRenderDoc = true;
#else
	const bool useVulkanDebugMarkersRenderDoc = false;
#endif

// #define SHOW_INFO_MESSAGES 0

//#define VSYNC 0 // 0 = no vsync, 1 = fifo, 2 = mailbox
//#define DISCRETE_GPU 0

struct QueueFamilyIndices 
{
	u32 graphicsFamily = ~0u;
	u32 presentFamily = ~0u;
	u32 computeFamily = ~0u;
	u32 transferFamily = ~0u;

	bool isValid() 
	{
		return (computeFamily != ~0u && transferFamily != ~0u && graphicsFamily != ~0u && presentFamily != ~0u);
	}
};

struct DeviceWithQueues
{
	VkPhysicalDevice physicalDevice = nullptr;
	VkDevice device = nullptr;
	VkSurfaceKHR surface = nullptr;
	
	VkCommandPool mainCommandPool = nullptr;
	VkCommandBuffer mainCommandBuffer = nullptr;

	VkQueue graphicsQueue = nullptr;
	VkQueue presentQueue = nullptr;
	VkQueue computeQueue = nullptr;

	QueueFamilyIndices queueFamilyIndices;

	VkFormat computeColorFormat = VkFormat::VK_FORMAT_UNDEFINED;
	VkFormat colorFormat = VkFormat::VK_FORMAT_UNDEFINED;
	VkFormat depthFormat = VkFormat::VK_FORMAT_UNDEFINED;
	VkColorSpaceKHR colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_END_RANGE_KHR;
};


QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
VkDebugUtilsMessengerEXT registerDebugCallback(VkInstance instance);

VkInstance createInstance();
VkPhysicalDevice createPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
DeviceWithQueues createDeviceWithQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


void setObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, std::string_view name);
void setObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);
void beginDebugRegion(VkCommandBuffer cmdbuffer, std::string_view pMarkerName, Vec4 color);
void insertDebugRegion(VkCommandBuffer cmdbuffer, std::string_view markerName, Vec4 color);
void endDebugRegion(VkCommandBuffer cmdBuffer);
