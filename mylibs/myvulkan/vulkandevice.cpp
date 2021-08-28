#include "vulkandevice.h"

#include <GLFW/glfw3.h>
#include "core/mytypes.h"
#include "vulkanswapchain.h"
#include "math/vector3.h"
#include <vulkan/vulkan_core.h>

#include <string>
#include <set>
#include <vector>
#include <memory.h>



static PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag = nullptr;
static PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = nullptr;
static PFN_vkCmdDebugMarkerBeginEXT	pfnCmdDebugMarkerBegin = nullptr;
static PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd = nullptr;
static PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert = nullptr;

struct VulkanDeviceOptionals
{
	bool canUseVulkanRenderdocExtensionMarker = false;
};


std::vector<const char *>validationLayers = 
{
	"VK_LAYER_KHRONOS_validation"
};

std::vector<const char *>deviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	//VK_KHR_MAINTENANCE1_EXTENSION_NAME
//	VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
};

static VulkanDeviceOptionals getDeviceOptionals(VkPhysicalDevice physicalDevice)
{
	u32 extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	VulkanDeviceOptionals result;

	for (const auto& extension : availableExtensions)
	{
		//printf("%s\n", extension.extensionName);
		if(useVulkanDebugMarkersRenderDoc && strcmp(extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0)
			result.canUseVulkanRenderdocExtensionMarker = true;
	}

	return result;
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
	bool errorMsg = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0 || (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0; 
	bool warningMsg = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0 || (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0;

#if !SHOW_INFO_MESSAGES	
	if(!errorMsg && !warningMsg)
		return VK_FALSE;
#endif
	const char *type = errorMsg ? "Error" : (warningMsg  ? "Warning" : "Info");
	
	printf("Type:%s, message: %s\n\n", type, pCallbackData->pMessage);
	if(errorMsg)
	{
		ASSERT(!"Validation error encountered!");
	}
	return VK_FALSE;
}


static std::vector<const char*> getRequiredExtensions() 
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	//extensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		//extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	return extensions;
}


static void acquireDeviceDebugRenderdocFunctions(VkDevice device)
{
	pfnDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
	pfnDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
	pfnCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
	pfnCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
	pfnCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) 
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{


		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;
		else 
			continue;
		if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			indices.transferFamily = i;
		else 
			continue;
		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			indices.computeFamily = i;
		else 
			continue;
	
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

		if (!presentSupport)
			continue;

		indices.presentFamily = i;

		if (indices.isValid())
			break;

		i++;
	}
	return indices;
}



// From sasha wilems debugmarker
void setObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, std::string_view name)
{
	// Check for a valid function pointer
	if (pfnDebugMarkerSetObjectName)
	{
		VkDebugMarkerObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objectType;
		nameInfo.object = object;
		nameInfo.pObjectName = name.data();
		pfnDebugMarkerSetObjectName(device, &nameInfo);
	}
}
 
void setObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
	if (pfnDebugMarkerSetObjectName)
	{
		VkDebugMarkerObjectTagInfoEXT tagInfo = {};
		tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
		tagInfo.objectType = objectType;
		tagInfo.object = object;
		tagInfo.tagName = name;
		tagInfo.tagSize = tagSize;
		tagInfo.pTag = tag;
		pfnDebugMarkerSetObjectTag(device, &tagInfo);
	}
}

// Start a new debug marker region
void beginDebugRegion(VkCommandBuffer cmdbuffer, std::string_view pMarkerName, Vec4 color)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
//	if (pfnCmdDebugMarkerBegin)
//	{
//		VkDebugMarkerMarkerInfoEXT markerInfo = {};
//		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
//		memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
//		markerInfo.pMarkerName = pMarkerName;
//		pfnCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
//	}
}

// Insert a new debug marker into the command buffer
void insertDebugRegion(VkCommandBuffer cmdbuffer, std::string_view markerName, Vec4 color)
{
	// Check for valid function pointer (may not be present if not running in a debugging application)
//	if (pfnCmdDebugMarkerInsert)
//	{
//		VkDebugMarkerMarkerInfoEXT markerInfo = {};
//		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
//		memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
//		markerInfo.pMarkerName = markerName.c_str();
//		pfnCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
//	}
}

// End the current debug marker region
void endDebugRegion(VkCommandBuffer cmdBuffer)
{
	// Check for valid function (may not be present if not runnin in a debugging application)
//	if (pfnCmdDebugMarkerEnd)
//	{
//		pfnCmdDebugMarkerEnd(cmdBuffer);
//	}
}

VkDebugUtilsMessengerEXT registerDebugCallback(VkInstance instance)
{
	if (!enableValidationLayers)
		return 0;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugReportCallback;

	VkDebugUtilsMessengerEXT debugMessenger = 0;

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	ASSERT(func);

	VK_CHECK(func(instance, &createInfo, nullptr, &debugMessenger));

	return debugMessenger;
}



VkInstance createInstance()
{
	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = VK_API_VERSION_1_1;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);


	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions = getRequiredExtensions();

	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledExtensionCount = (u32)extensions.size();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	if (enableValidationLayers)
	{
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.enabledLayerCount = u32(validationLayers.size());

		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugReportCallback;

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	} 
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	VkInstance instance = nullptr;
	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));

	return instance;
}





VkPhysicalDevice createPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
	VkPhysicalDevice devices[256] = {};
	u32 count = ARRAYSIZE(devices);

	VK_CHECK(vkEnumeratePhysicalDevices(instance, &count, devices));

	VkPhysicalDevice primary = nullptr;
	VkPhysicalDevice secondary = nullptr;

	for(u32 i = 0; i < count; ++i)
	{
		VkPhysicalDeviceProperties prop;
		VkPhysicalDevice physicalDevice = devices[i];
		vkGetPhysicalDeviceProperties(physicalDevice, &prop);

		if(prop.apiVersion < VK_API_VERSION_1_1)
			continue;
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
		bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		if(!swapChainAdequate)
			continue;

		uint32_t formatIndex = ~0u;
		for (uint32_t j = 0; j < ARRAYSIZE(defaultFormat); ++j)
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, defaultFormat[j], &formatProperties);
			if((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0u &&
				 (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0u)
			{
				formatIndex = j;
				goto formatIndexFound;
	
			}
		}
		formatIndexFound:

		if(formatIndex == ~0u)
			continue;
		ASSERT(formatIndex != ~0u);
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
		if(!indices.isValid())
			continue;
		bool extensionsSupported = false;
		{
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

			std::set<std::string> requiredExtensions;
			for (const char *str : deviceExtensions)
			{
				requiredExtensions.insert(str);
			}

			for (const auto& extension : availableExtensions)
			{
				requiredExtensions.erase(extension.extensionName);
			}

			extensionsSupported = requiredExtensions.empty();

			
		}
		if(!extensionsSupported)
			continue;
	
		#if DISCRETE_GPU
			static constexpr VkPhysicalDeviceType wantedDevice = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
		#else
			static constexpr VkPhysicalDeviceType wantedDevice = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
		#endif

		if(prop.deviceType == wantedDevice)
		{
			primary = secondary = devices[i];
			break;
		}
		else if(!secondary && 
			(prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
			prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
		{
			secondary = devices[i];
		}
	}
	if(!primary && !secondary)
	{
		printf("Didn't find any gpus\n");
		return nullptr;
	}


	VkPhysicalDeviceProperties prop;
	if(primary)
	{
		vkGetPhysicalDeviceProperties(primary, &prop);
	}
	else if(secondary)
	{
		vkGetPhysicalDeviceProperties(secondary, &prop);
	}
	const char *typeText = prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated";
	printf("Picking %s device: %s\n", typeText, prop.deviceName);
	return primary ? primary : secondary;


}



DeviceWithQueues createDeviceWithQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	DeviceWithQueues deviceWithQueues;

	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
	ASSERT(queueFamilyIndices.isValid());

	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
	bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	if(!swapChainAdequate)
		return deviceWithQueues;


	for(uint32_t i = 0; i < swapChainSupport.formats.size() && deviceWithQueues.colorFormat == VK_FORMAT_UNDEFINED; ++i)
	{
		if(swapChainSupport.formats[i].colorSpace != defaultColorSpace)
			continue;

		for (uint32_t j = 0; j < ARRAYSIZE(defaultFormat); ++j)
		{
			if(swapChainSupport.formats[i].format == defaultFormat[j])
			{
				deviceWithQueues.colorFormat = defaultFormat[j];

			}
		}
	}

	if(deviceWithQueues.colorFormat == VK_FORMAT_UNDEFINED && swapChainSupport.formats.size() == 1 && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
	{
		deviceWithQueues.colorFormat = defaultFormat[0];
	}
	ASSERT(deviceWithQueues.colorFormat != VK_FORMAT_UNDEFINED);


	for (uint32_t j = 0; j < ARRAYSIZE(defaultFormat); ++j)
	{
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, defaultFormat[j], &formatProperties);
		if((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0u &&
			 (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0u)
		{
			deviceWithQueues.computeColorFormat = defaultFormat[j];
			break;
		}
	}

	ASSERT(deviceWithQueues.computeColorFormat != VK_FORMAT_UNDEFINED);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_FALSE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;


	std::vector<const char *> deviceExts = deviceExtensions;
	
	VulkanDeviceOptionals optionals = getDeviceOptionals(physicalDevice);
	if(optionals.canUseVulkanRenderdocExtensionMarker)
	{
		deviceExts.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
	}
	createInfo.enabledExtensionCount = u32(deviceExts.size());
	createInfo.ppEnabledExtensionNames = deviceExts.data();


	if (enableValidationLayers) 
	{
		createInfo.enabledLayerCount = u32(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}
	
	VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &deviceWithQueues.device));
	ASSERT(deviceWithQueues.device);

	vkGetDeviceQueue(deviceWithQueues.device, queueFamilyIndices.graphicsFamily, 0, &deviceWithQueues.graphicsQueue);
	vkGetDeviceQueue(deviceWithQueues.device, queueFamilyIndices.presentFamily, 0, &deviceWithQueues.presentQueue);
	vkGetDeviceQueue(deviceWithQueues.device, queueFamilyIndices.computeFamily, 0, &deviceWithQueues.computeQueue);
	ASSERT(deviceWithQueues.graphicsQueue);
	ASSERT(deviceWithQueues.presentQueue);
	ASSERT(deviceWithQueues.computeQueue);

	if(optionals.canUseVulkanRenderdocExtensionMarker)
		acquireDeviceDebugRenderdocFunctions(deviceWithQueues.device);
	deviceWithQueues.queueFamilyIndices = queueFamilyIndices;
	deviceWithQueues.colorSpace = defaultColorSpace;
	deviceWithQueues.depthFormat = defaultDepthFormat;
	deviceWithQueues.surface = surface;
	deviceWithQueues.physicalDevice = physicalDevice;
	return deviceWithQueues; 
}


