#pragma once

#include <string>
#include <vulkan/vulkan_core.h>

#include <core/mytypes.h>
#include <math/vector3.h>


#ifdef NDEBUG
	const bool useVulkanDebugMarkersRenderDoc = false;
#else
	const bool useVulkanDebugMarkersRenderDoc = true;
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
	VkDevice device = nullptr;
	VkQueue graphicsQueue = nullptr;
	VkQueue presentQueue = nullptr;
	QueueFamilyIndices queueFamilyIndices;
	VkFormat computeColorFormat = VkFormat::VK_FORMAT_UNDEFINED;
	VkFormat colorFormat = VkFormat::VK_FORMAT_UNDEFINED;
	VkFormat depthFormat = VkFormat::VK_FORMAT_UNDEFINED;
	VkColorSpaceKHR colorSpace;
};


QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
VkDebugUtilsMessengerEXT registerDebugCallback(VkInstance instance);

VkInstance createInstance();
VkPhysicalDevice createPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
DeviceWithQueues createDeviceWithQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


void setObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name);
void setObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag);
void beginDebugRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, Vec4 color);
void insertDebugRegion(VkCommandBuffer cmdbuffer, std::string markerName, Vec4 color);
void endDebugRegion(VkCommandBuffer cmdBuffer);
