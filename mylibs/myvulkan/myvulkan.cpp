#include "myvulkan.h"

#include <GLFW/glfw3.h>
#include "core/general.h"
#include "core/mytypes.h"

#include "math/vector3.h"

#include <vulkan/vulkan_core.h>

#include <string>
#include <set>
#include <vector>
#include <memory.h>


// Global variables
VkDebugUtilsMessengerEXT debugCallBack = nullptr;
VkInstance instance = nullptr;
VkPhysicalDevice physicalDevice = nullptr;
VkDevice device = nullptr;
VkSurfaceKHR surface = nullptr;

VkPhysicalDeviceMemoryProperties memoryProperties;

VkQueue graphicsQueue = nullptr;
VkQueue presentQueue = nullptr;
VkQueue computeQueue = nullptr;

QueueFamilyIndices queueFamilyIndices;



Buffer scratchBuffer;
Buffer renderFrameBuffer;

// Do I need this??
VkRenderPass renderPass = nullptr;

SwapChain swapchain;

VkQueryPool queryPool = nullptr;

VkSemaphore acquireSemaphore = nullptr;
VkSemaphore releaseSemaphore = nullptr;

VkFence fence = nullptr;
bool waitForFence = true;
VkCommandPool commandPool = nullptr;

VkCommandBuffer commandBuffer = nullptr;
VkFramebuffer targetFB = nullptr;


VkPipelineCache pipelineCache = nullptr;


VkFormat computeColorFormat = VkFormat::VK_FORMAT_UNDEFINED;
VkFormat colorFormat = VkFormat::VK_FORMAT_UNDEFINED;
VkFormat depthFormat = VkFormat::VK_FORMAT_UNDEFINED;
VkColorSpaceKHR colorSpace = VkColorSpaceKHR::VK_COLOR_SPACE_MAX_ENUM_KHR;

SwapChain swapChain;

Image mainColorRenderTarget;
Image mainDepthRenderTarget;

bool needToResize = false;
uint32_t imageIndex = 0u;

//QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
//static bool createInstance();
//static bool createPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
//static bool createDeviceWithQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


static PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag = nullptr;
static PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = nullptr;
static PFN_vkCmdDebugMarkerBeginEXT    pfnCmdDebugMarkerBegin = nullptr;
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
//    VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
};

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

VkDebugUtilsMessengerEXT registerDebugCallback(VkInstance instance)
{
    if (!enableValidationLayers)
        return nullptr;

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




static u32 selectMemoryType(const VkPhysicalDeviceMemoryProperties &memoryProperties, u32 memoryTypeBits, VkMemoryPropertyFlags flags)
{
    for(u32 i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        bool isUsed = (memoryTypeBits & (1 << i)) != 0u;
        u32 foundFlags = (memoryProperties.memoryTypes[i].propertyFlags & flags);

        //printf("Memory %u: Used: %u, flags: %u\n", i, isUsed, foundFlags);

        if(isUsed && foundFlags == flags)
            return i;
    }
//1, 3, 6,7, 10, 11
    ASSERT(!"No compatible memory found!");
    return ~0u;
}

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


static std::vector<const char*> getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0u;
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






static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
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




static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    ASSERT(formatCount > 0);

    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    ASSERT(presentModeCount > 0);

    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());

    return details;
}




static void destroySwapchain(SwapChain &swapchain)
{
    swapchain.images.clear();
    swapchain.swapchainCount = 0u;
    swapchain.width = swapchain.height = 0u;

    if(swapchain.swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);
    swapchain.swapchain = VK_NULL_HANDLE;
}

static bool createSwapchain(GLFWwindow *window)
{
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
        ASSERT(swapChainSupport.formats.size() > 0);
        VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
        bool found = false;
        for (const auto& availableFormat : swapChainSupport.formats)
        {
            if (availableFormat.format == colorFormat && availableFormat.colorSpace == colorSpace)
            {
                surfaceFormat = availableFormat;
                found = true;
                break;
            }
        }
        if(!found && swapChainSupport.formats.size() == 1 && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
        {
            surfaceFormat.colorSpace = colorSpace;
            surfaceFormat.format = colorFormat;
            found = true;
        }
        ASSERT(found);

        VkPresentModeKHR presentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
        #if VSYNC == 1
            VkPresentModeKHR searchMode = presentMode;
        #elif VSYNC == 2
            VkPresentModeKHR searchMode = VK_PRESENT_MODE_MAILBOX_KHR;
        #else
            VkPresentModeKHR searchMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        #endif

        for (const auto& availablePresentMode : swapChainSupport.presentModes)
        {
            if (availablePresentMode == searchMode)
            {
                presentMode = availablePresentMode;
                break;
            }
        }

        VkExtent2D extent = swapChainSupport.capabilities.currentExtent;
        if (swapChainSupport.capabilities.currentExtent.width == UINT32_MAX)
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            extent.width = u32(width);
            extent.height = u32(height);

            extent.width = std::max(swapChainSupport.capabilities.minImageExtent.width,
                std::min(swapChainSupport.capabilities.maxImageExtent.width, extent.width));
            extent.height = std::max(swapChainSupport.capabilities.minImageExtent.height,
                std::min(swapChainSupport.capabilities.maxImageExtent.height, extent.height));
        }

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.oldSwapchain = swapChain.swapchain;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };
        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            // SETTING THIS TO 1 + giving queuefamilyindices pointer CAUSED CRASH WITH VKKHRSWAPCHAINCREATION WITH AMD
            createInfo.queueFamilyIndexCount = 0;
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        VkSwapchainKHR swapchain = 0;
    //    PreCallValidateCreateSwapchainKHR()
        [[maybe_unused]] VkResult res = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);
        if (res != VK_SUCCESS)
        {
            LOG("Failed to initialize swapchain\n");
            return false;
        }
        VK_CHECK(res);
        swapChain.swapchain = swapchain;
    }




    u32 swapchainCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapChain.swapchain, &swapchainCount, nullptr));

    swapChain.swapchainCount = swapchainCount;
    swapChain.images.resize(swapchainCount);
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapChain.swapchain, &swapchainCount, swapChain.images.data()));

    i32 width = 0;
    i32 height = 0;

    glfwGetWindowSize(window, &width, &height);
    swapChain.width = u32(width);
    swapChain.height = u32(height);
    return true;
}










static bool createInstance()
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

    instance = nullptr;
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));

    return instance != nullptr;
}





static bool createPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
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

        if(!prop.limits.timestampComputeAndGraphics)
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
        return false;
    }

    physicalDevice = primary ? primary : secondary;

    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(secondary, &prop);

    const char *typeText = prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated";
    printf("Picking %s device: %s\n", typeText, prop.deviceName);
    return true;
}



static bool createDeviceWithQueues(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
    ASSERT(queueFamilyIndices.isValid());

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    if(!swapChainAdequate)
        return false;

    colorFormat = VK_FORMAT_UNDEFINED;

    for(uint32_t i = 0; i < swapChainSupport.formats.size() && colorFormat == VK_FORMAT_UNDEFINED; ++i)
    {
        if(swapChainSupport.formats[i].colorSpace != defaultColorSpace)
            continue;

        for (uint32_t j = 0; j < ARRAYSIZE(defaultFormat); ++j)
        {
            if(swapChainSupport.formats[i].format == defaultFormat[j])
            {
                colorFormat = defaultFormat[j];

            }
        }
    }

    if(colorFormat == VK_FORMAT_UNDEFINED && swapChainSupport.formats.size() == 1 && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
    {
        colorFormat = defaultFormat[0];
    }
    ASSERT(colorFormat != VK_FORMAT_UNDEFINED);


    for (uint32_t j = 0; j < ARRAYSIZE(defaultFormat); ++j)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, defaultFormat[j], &formatProperties);
        if((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0u &&
             (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0u)
        {
            computeColorFormat = defaultFormat[j];
            break;
        }
    }

    ASSERT(computeColorFormat != VK_FORMAT_UNDEFINED);

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

    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
    ASSERT(device);

    vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilyIndices.presentFamily, 0, &presentQueue);
    vkGetDeviceQueue(device, queueFamilyIndices.computeFamily, 0, &computeQueue);
    ASSERT(graphicsQueue);
    ASSERT(presentQueue);
    ASSERT(computeQueue);

    if(optionals.canUseVulkanRenderdocExtensionMarker)
        acquireDeviceDebugRenderdocFunctions(device);
    colorSpace = defaultColorSpace;
    depthFormat = defaultDepthFormat;
    surface = surface;
    physicalDevice = physicalDevice;
    return true;
}





bool resizeSwapchain(GLFWwindow *window)
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
    }

    uint32_t newWidth = uint32_t(width);
    uint32_t newHeight = uint32_t(height);

    if (swapChain.width == newWidth && swapChain.height == newHeight)
        return false;

    VK_CHECK(vkDeviceWaitIdle(device));

    SwapChain oldSwapchain = swapChain;
    createSwapchain(window);

    destroySwapchain(oldSwapchain);
    return true;
}






static VkSemaphore createSemaphore(VkDevice device)
{
    VkSemaphore semaphore = 0;
    VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore));
    return semaphore;
}

static VkCommandPool createCommandPool(VkDevice device, u32 familyIndex)
{
    VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolCreateInfo.queueFamilyIndex = familyIndex;

    VkCommandPool commandPool = 0;
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

    VkRenderPass renderPass = 0;

    VK_CHECK(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass));
    return renderPass;
}



static VkQueryPool createQueryPool(VkDevice device, u32 queryCount)
{
    VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };

    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = queryCount;

    VkQueryPool pool = 0;
    VK_CHECK(vkCreateQueryPool(device, &createInfo, nullptr, &pool));

    ASSERT(pool);
    return pool;
}


static VkFence createFence(VkDevice device)
{
    VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence result = 0;
    VK_CHECK(vkCreateFence(device, &createInfo, nullptr, &result));
    ASSERT(result);
    return result;
}




















bool initVulkan(GLFWwindow *window)
{
    ASSERT(window);
    if(!window)
    {
        printf("Empty window!\n");
        return false;
    }
    if(!createInstance())
    {
        printf("Failed to create vulkan instance!\n");
        return false;
    }

    debugCallBack = registerDebugCallback(instance);

    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    ASSERT(surface);
    if(!surface)
    {
        printf("Failed to create vulkan surface!\n");
        return false;
    }


    if(!createPhysicalDevice(instance, surface))
    {
        printf("Failed to create vulkan physical device!\n");
        return false;
    }
    ASSERT(physicalDevice);


    if(!createDeviceWithQueues(physicalDevice, surface))
    {
        printf("Failed to create vulkan device!\n");
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
    renderPass = createRenderPass(device, computeColorFormat, depthFormat);
    ASSERT(renderPass);
    if(!renderPass)
    {
        printf("Failed to create render pass!\n");
        return false;
    }

    [[maybe_unused]] bool scSuccess = createSwapchain(window);
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

    commandPool = createCommandPool(device, queueFamilyIndices.graphicsFamily);
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


    {
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);


        scratchBuffer = createBuffer(64 * 1024 * 1024, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Scratch buffer");

    }
    setObjectName(device, (uint64_t)commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "Main command buffer");

    {
        // Create buffers
        renderFrameBuffer = createBuffer(64u * 1024 * 1024,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Frame render uniform buffer");

    }


    return true;
}

static void deleteFrameTargets()
{
    vkDestroyFramebuffer(device, targetFB, nullptr);
    targetFB = nullptr;
    destroyImage(mainColorRenderTarget);
    destroyImage(mainDepthRenderTarget);

    //QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
    //deviceWithQueues.queueFamilyIndices = queueFamilyIndices;
    //ASSERT(deviceWithQueues.queueFamilyIndices.isValid());

}

static bool createGraphics()
{
    //vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    //recreateSwapchainData();

    // create color and depth images
    {
        mainColorRenderTarget =
            createImage(
                swapchain.width, swapchain.height,
                //deviceWithQueues.computeColorFormat,
                colorFormat,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                //| VK_IMAGE_USAGE_STORAGE_BIT
                , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                "Main color target image");

        mainDepthRenderTarget = createImage(
            swapchain.width, swapchain.height, depthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            "Main depth target image");

        targetFB = createFramebuffer(renderPass,
            mainColorRenderTarget.imageView, mainDepthRenderTarget.imageView,
            swapchain.width, swapchain.height);
    }
    return true;
}







static VkDescriptorSetLayout createSetLayout(const std::vector<DescriptorSetLayout> &descriptors, VkShaderStageFlags stage)
{
    ASSERT(descriptors.size() > 0);
    std::vector<VkDescriptorSetLayoutBinding> setBindings(descriptors.size());
    for(u32 i = 0; i < u32(descriptors.size()); ++i)
    {
        setBindings[i].binding = descriptors[i].bindingIndex;
        setBindings[i].descriptorType = descriptors[i].descriptorType; // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        setBindings[i].descriptorCount = 1;
        setBindings[i].stageFlags = stage; // VK_SHADER_STAGE_VERTEX_BIT;
    }

    VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
//    createInfo.flags =  VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    createInfo.bindingCount = u32(setBindings.size());
    createInfo.pBindings = setBindings.data();

    VkDescriptorSetLayout setLayout = 0;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &setLayout));
    ASSERT(setLayout);
    return setLayout;
}

Pipeline createPipelineLayout(const std::vector<DescriptorSetLayout> &descriptors, VkShaderStageFlags stage)
{
    VkDescriptorSetLayout setLayout = 0;
    VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

    if(descriptors.size() > 0)
        setLayout = createSetLayout(descriptors, stage);


    createInfo.setLayoutCount = setLayout ? 1 : 0;
    createInfo.pSetLayouts = setLayout ? &setLayout : nullptr;

    VkPipelineLayout layout = 0;
    VK_CHECK(vkCreatePipelineLayout(device, &createInfo, nullptr, &layout));
    ASSERT(layout);

    //vkDestroyDescriptorSetLayout(device, setLayout, nullptr);

    Pipeline result;
    result.pipelineLayout = layout;
    result.descriptorSetLayout = setLayout;
    return result;
}

static void recreateSwapchainData()
{
    deleteFrameTargets();
    createGraphics();
    needToResize = false;
}



















void beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
{
    VK_CHECK(vkResetCommandPool(device, commandPool, 0));

    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    return;
}

void endSingleTimeCommands(VkDevice device, VkCommandBuffer commandBuffer, VkQueue queue)
{
    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(queue));
}








Image createImage(u32 width, u32 height, VkFormat format,
    VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char *imageName)
{

    VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.extent = { width, height, 1 };
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = usage;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &queueFamilyIndices.graphicsFamily;
    Image result;

    VK_CHECK(vkCreateImage(device, &createInfo, nullptr, &result.image));
    ASSERT(result.image);

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device, result.image, &memoryRequirements);

    u32 memoryTypeIndex = selectMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags);
    ASSERT(memoryTypeIndex != ~0u);

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &result.deviceMemory));
    ASSERT(result.deviceMemory);

    VK_CHECK(vkBindImageMemory(device, result.image, result.deviceMemory, 0));

    result.imageView = createImageView(result.image, format);
    ASSERT(result.imageView);

/*
    // If cpu image access?
    void *data = nullptr;
    if(memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        VK_CHECK(vkMapMemory(device, result.deviceMemory, 0, size, 0, &data));
        ASSERT(data);
    }
    result.data = data;
    result.size = size;
*/
    setObjectName(device, (uint64_t)result.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, imageName);
    result.imageName = imageName;

    return result;
}



void updateImageWithData(u32 width, u32 height, u32 pixelSize,
    Image &targetImage,
    u32 dataSize, void *data)
{
    ASSERT(data != nullptr || dataSize > 0u);
    ASSERT(scratchBuffer.data);
    ASSERT(scratchBuffer.size >= width * height * pixelSize);
    ASSERT(targetImage.image);

    uint32_t offset = 0u;
    offset = uploadToScratchbuffer(data, dataSize, offset);
    beginSingleTimeCommands();
    {
        VkImageMemoryBarrier imageBarriers[] =
        {
            imageBarrier(targetImage,
                        VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
        };

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(imageBarriers), imageBarriers);
    }

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, scratchBuffer.buffer, targetImage.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    {
        VkImageMemoryBarrier imageBarriers[] =
        {
            imageBarrier(targetImage,
                        VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL),
        };

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(imageBarriers), imageBarriers);
    }
    endSingleTimeCommands();

    return;
}

VkSampler createSampler(const VkSamplerCreateInfo &info)
{
    VkSampler sampler = nullptr;
    VK_CHECK(vkCreateSampler(device, &info, nullptr, &sampler));
    return sampler;
}

void destroyImage(Image &image)
{
    vkDestroyImageView(device, image.imageView, nullptr);
    vkDestroyImage(device, image.image, nullptr);
    vkFreeMemory(device, image.deviceMemory, nullptr);

    image = Image{};
}


void destroySampler(VkSampler sampler)
{
    vkDestroySampler(device, sampler, nullptr);
}

void destroyShaderModule(VkShaderModule shaderModule)
{
    vkDestroyShaderModule(device, shaderModule, nullptr);
}


Buffer createBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char *bufferName)
{
    Buffer result;

    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = size;
    createInfo.usage = usage;

    VK_CHECK(vkCreateBuffer(device, &createInfo, nullptr, &result.buffer));
    ASSERT(result.buffer);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, result.buffer, &memoryRequirements);

    u32 memoryTypeIndex = selectMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags);
    ASSERT(memoryTypeIndex != ~0u);

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &result.deviceMemory));
    ASSERT(result.deviceMemory);

    VK_CHECK(vkBindBufferMemory(device, result.buffer, result.deviceMemory, 0));

    void *data = nullptr;
    if(memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        VK_CHECK(vkMapMemory(device, result.deviceMemory, 0, size, 0, &data));
        ASSERT(data);
    }
    result.data = data;
    result.size = size;


    setObjectName(device, (uint64_t)result.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, bufferName);
    result.bufferName = bufferName;

    return result;
}

size_t uploadToScratchbuffer(void* data, size_t size, size_t offset)
{
    ASSERT(scratchBuffer.data);
    ASSERT(size);
    ASSERT(scratchBuffer.size >= size);

    memcpy((unsigned char*)scratchBuffer.data + offset, data, size);
    offset += size;

    return offset;
}

void uploadBufferToImage(Image &gpuImage, Buffer &scratchBuffer,
                         uint32_t width, uint32_t height, uint32_t bytesPerPixel, uint32_t bufferOffset)
{


    //    endSingleTimeCommands(device, commandBuffer, queue);

}



void uploadScratchBufferToGpuBuffer(Buffer &gpuBuffer, size_t size)
{
    ASSERT(scratchBuffer.data);
    ASSERT(scratchBuffer.size >= size);
    ASSERT(gpuBuffer.size >= size);

    beginSingleTimeCommands(device, commandPool, commandBuffer);

    VkBufferCopy region = { 0, 0, VkDeviceSize(size) };
    vkCmdCopyBuffer(commandBuffer, scratchBuffer.buffer, gpuBuffer.buffer, 1, &region);

    VkBufferMemoryBarrier copyBarrier = bufferBarrier(gpuBuffer.buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, size);
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyBarrier, 0, nullptr);

    endSingleTimeCommands(device, commandBuffer, graphicsQueue);

}

void destroyBuffer(Buffer &buffer)
{
    vkDestroyBuffer(device, buffer.buffer, nullptr);
    vkFreeMemory(device, buffer.deviceMemory, nullptr);
    buffer.buffer = 0;
    buffer.data = nullptr;
    buffer.deviceMemory = 0;
    buffer.size = 0ll;
}


bool startRender(GLFWwindow *window)
{
    VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
    if (acquireSemaphore == VK_NULL_HANDLE)
        return false;
    VkResult res = ( vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &imageIndex) );
    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if (resizeSwapchain(window))
        {
            recreateSwapchainData();
            VK_CHECK(vkDeviceWaitIdle(device));
            needToResize = false;
        }
        return false;
    }
    else if (res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR)
        return true;

    VK_CHECK(res);
    return false;
}

/*
uint32_t updateRenderFrameBuffer()
{
    struct Buff
    {
        Vector2 areaSize;
        float tmp[6 + 8];
    };

    Buff buff{ Vector2(windowWidth, windowHeight) };

    // use scratch buffer to unifrom buffer transfer
    uint32_t buffSize = uint32_t(sizeof(Buff));
    memcpy(scratchBuffer.data, &buff, buffSize);
    {
        VkBufferCopy region = { 0, 0, VkDeviceSize(buffSize) };
        vkCmdCopyBuffer(commandBuffer, scratchBuffer.buffer, renderFrameBuffer.buffer, 1, &region);
    }

    VkBufferMemoryBarrier bar[]
    {
        bufferBarrier(renderFrameBuffer.buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, buffSize),
    };

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, bar, 0, nullptr);

    uint32_t offset = fontSystem.update(deviceWithQueues.device, commandBuffer, renderPass, Vector2(windowWidth, windowHeight), scratchBuffer, buffSize);

    return offset;
}
*/
void present(GLFWwindow *window, Image &presentImage)
{

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
        VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence));

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &releaseSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.swapchain;
        presentInfo.pImageIndices = &imageIndex;

        VkResult res = ( vkQueuePresentKHR(presentQueue, &presentInfo) );
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || needToResize)
        {
            needToResize = true;
            if (resizeSwapchain(window))
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

}


VkFramebuffer createFramebuffer(VkRenderPass renderPass,
    VkImageView colorView, VkImageView depthView, u32 width, u32 height)
{
    VkImageView attachMents[] = { colorView, depthView };
    VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = 2;
    createInfo.layers = 1;
    createInfo.pAttachments = attachMents;
    createInfo.width = width;
    createInfo.height = height;

    VkFramebuffer framebuffer = 0;
    VK_CHECK(vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffer));
    return framebuffer;
}

VkImageView createImageView(VkImage image, VkFormat format)
{
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if(format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
        format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D16_UNORM_S8_UINT ||
        format == VK_FORMAT_S8_UINT
    )
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;

    createInfo.components.r = VK_COMPONENT_SWIZZLE_R; // VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_G; //VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_B; //VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_A; //VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;


    VkImageView view = 0;
    VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &view));

    ASSERT(view);
    return view;
}


VkImageMemoryBarrier imageBarrier(Image &image,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask)
{
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcAccessMask = image.accessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = image.layout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.image;
    barrier.subresourceRange.aspectMask = aspectMask;
    // Andoird error?
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;


    image.accessMask = dstAccessMask;
    image.layout = newLayout;
    return barrier;
}


VkImageMemoryBarrier imageBarrier(VkImage image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask)
{
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    // Andoird error?
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return barrier;
}

VkBufferMemoryBarrier bufferBarrier(VkBuffer buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, size_t size, size_t offset)
{
    ASSERT(size > 0);
    VkBufferMemoryBarrier copyBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
    copyBarrier.srcAccessMask = srcAccessMask;
    copyBarrier.dstAccessMask = dstAccessMask;
    copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copyBarrier.buffer = buffer;
    copyBarrier.offset = offset;
    copyBarrier.size = size;
    return copyBarrier;
}

VkBufferMemoryBarrier bufferBarrier(const Buffer &buffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
{
    return bufferBarrier(buffer.buffer, srcAccessMask, dstAccessMask, buffer.size);
}
























Descriptor createDescriptor(const std::vector<DescriptorSetLayout> &descriptors, VkDescriptorSetLayout descriptorSetLayout)
{
    Descriptor result;
    if(descriptors.size() == 0)
        return result;

    std::vector<VkDescriptorPoolSize> poolSizes(descriptors.size());

    for(u32 i = 0; i < descriptors.size(); ++i)
    {
        poolSizes[i].type = descriptors[i].descriptorType;
        poolSizes[i].descriptorCount = 1;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = u32(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1; // NOTE ????

    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &result.pool));

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = result.pool;
    allocInfo.descriptorSetCount = 1u;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet = 0;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
    ASSERT(descriptorSet);

    result.descriptorSet = descriptorSet;
    return result;
}

bool setBindDescriptorSet(const std::vector<DescriptorSetLayout> &descriptors,
    const std::vector<DescriptorInfo> &descriptorInfos, VkDescriptorSet descriptorSet)
{
    u32 writeDescriptorCount = (u32)descriptorInfos.size();
    if(writeDescriptorCount < 1u)
        return false;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets(writeDescriptorCount);
    std::vector<VkDescriptorBufferInfo> bufferInfos(writeDescriptorCount);

    std::vector<VkDescriptorImageInfo> imageInfos(writeDescriptorCount);

    u32 writeIndex = 0u;
    u32 bufferCount = 0u;
    u32 imageCount = 0u;
    for(u32 i = 0; i < descriptorInfos.size(); ++i)
    {
        if(descriptorInfos[ i ].type == DescriptorInfo::DescriptorType::BUFFER)
        {
            const VkDescriptorBufferInfo &bufferInfo = descriptorInfos[ i ].bufferInfo;
            ASSERT(bufferInfo.buffer);
            ASSERT(bufferInfo.range > 0u);
            bufferInfos[bufferCount] = {};
            bufferInfos[bufferCount].buffer = bufferInfo.buffer;
            bufferInfos[bufferCount].offset = bufferInfo.offset;
            bufferInfos[bufferCount].range = bufferInfo.range;
            writeDescriptorSets[writeIndex] = VkWriteDescriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            writeDescriptorSets[writeIndex].dstSet = descriptorSet;
            writeDescriptorSets[writeIndex].dstArrayElement = 0;
            writeDescriptorSets[writeIndex].descriptorType = descriptors[i].descriptorType;
            writeDescriptorSets[writeIndex].dstBinding = descriptors[i].bindingIndex;
            writeDescriptorSets[writeIndex].pBufferInfo = &bufferInfos[bufferCount];
            writeDescriptorSets[writeIndex].descriptorCount = 1;

            ++bufferCount;
            ++writeIndex;
        }
        else if(descriptorInfos[ i ].type == DescriptorInfo::DescriptorType::IMAGE)
        {
            const VkDescriptorImageInfo &imageInfo = descriptorInfos[ i ].imageInfo;
            ASSERT(imageInfo.sampler);
            ASSERT(imageInfo.imageView);
            ASSERT(imageInfo.imageLayout);

            imageInfos[imageCount] = {};
            imageInfos[imageCount].sampler = imageInfo.sampler;
            imageInfos[imageCount].imageView = imageInfo.imageView;
            imageInfos[imageCount].imageLayout = imageInfo.imageLayout;

            writeDescriptorSets[writeIndex] = VkWriteDescriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            writeDescriptorSets[writeIndex].dstSet = descriptorSet;
            writeDescriptorSets[writeIndex].dstArrayElement = 0;
            writeDescriptorSets[writeIndex].descriptorType = descriptors[i].descriptorType;
            writeDescriptorSets[writeIndex].dstBinding = descriptors[i].bindingIndex;
            writeDescriptorSets[writeIndex].pImageInfo = &imageInfos[imageCount];
            writeDescriptorSets[writeIndex].descriptorCount = 1;
            ++writeIndex;
            ++imageCount;
        }
        else
        {
            ASSERT(true);
            return false;
        }
    }

    if(writeDescriptorSets.size() > 0)
    {
        vkUpdateDescriptorSets(device, u32(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
    return true;
}


void destroyDescriptor(Descriptor &descriptor)
{
    vkDestroyDescriptorPool(device, descriptor.pool, nullptr);
    descriptor.pool = 0;
    descriptor.descriptorSet = 0;
}


VkPipeline createGraphicsPipeline(VkRenderPass renderPass, VkShaderModule vs, VkShaderModule fs, VkPipelineLayout pipelineLayout,
    bool depthTest)
{
    VkPipelineShaderStageCreateInfo stageInfos[2] = {};
    stageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stageInfos[0].module = vs;
    stageInfos[0].pName = "main";

    stageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stageInfos[1].module = fs;
    stageInfos[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    VkPipelineInputAssemblyStateCreateInfo assemblyInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportInfo.scissorCount = 1;
    viewportInfo.viewportCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterInfo.lineWidth = 1.0f;
    rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // VK_FRONT_FACE_CLOCKWISE;
    rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;

    VkPipelineMultisampleStateCreateInfo multiSampleInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multiSampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    if (depthTest)
    {
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_TRUE;
    }
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState colorAttachmentState = {};
    colorAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blendInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    blendInfo.pAttachments = &colorAttachmentState;
    blendInfo.attachmentCount = 1;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicInfo.pDynamicStates = dynamicStates;
    dynamicInfo.dynamicStateCount = ARRAYSIZE(dynamicStates);

    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    createInfo.stageCount = ARRAYSIZE(stageInfos);
    createInfo.pStages = stageInfos;
    createInfo.pVertexInputState = &vertexInfo;
    createInfo.pInputAssemblyState = &assemblyInfo;
    createInfo.pViewportState = &viewportInfo;
    createInfo.pRasterizationState = &rasterInfo;
    createInfo.pMultisampleState = &multiSampleInfo;
    createInfo.pDepthStencilState = &depthInfo;
    createInfo.pColorBlendState = &blendInfo;
    createInfo.pDynamicState = &dynamicInfo;
    createInfo.renderPass = renderPass;
    createInfo.layout = pipelineLayout;
    createInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline = 0;
    VK_CHECK(vkCreateGraphicsPipelines(device, nullptr, 1, &createInfo, nullptr, &pipeline));
    ASSERT(pipeline);

    return pipeline;
}


VkPipeline createComputePipeline(VkShaderModule cs, VkPipelineLayout pipelineLayout)
{
    //Pipeline result = createPipelineLayout(device, descriptors, pushConstantSize, pushConstantStage);
    //ASSERT(result.pipelineLayout);
    if(!pipelineLayout)
        return nullptr;

    VkPipelineShaderStageCreateInfo stageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = cs;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    createInfo.stage = stageInfo;
    createInfo.layout = pipelineLayout;

    VkPipeline pipeline = 0;
    VK_CHECK(vkCreateComputePipelines(device, nullptr, 1, &createInfo, nullptr, &pipeline));
    ASSERT(pipeline);

    return pipeline;
}


void destroyPipeline(Pipeline &pipeline)
{
    if(pipeline.pipeline)
        vkDestroyPipeline(device, pipeline.pipeline, nullptr);
    if(pipeline.pipelineLayout)
        vkDestroyPipelineLayout(device, pipeline.pipelineLayout, nullptr );

    if(pipeline.descriptorSetLayout)
        vkDestroyDescriptorSetLayout(device, pipeline.descriptorSetLayout, nullptr);

    pipeline = Pipeline {};
}






VkShaderModule loadShader(std::string_view filename)
{
    std::vector<char> buffer;
    VkShaderModule shaderModule = 0;

    if (loadBytes(filename, buffer))
    {
        ASSERT(buffer.size() % 4 == 0);

        VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = buffer.size();
        createInfo.pCode = reinterpret_cast<u32*>(buffer.data());
        vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    }
    ASSERT(shaderModule);
    return shaderModule;
}



















void deinitVulkan()
{
    if(device)
    {
        destroyBuffer(scratchBuffer);
        destroyBuffer(renderFrameBuffer);

        deleteFrameTargets();

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyQueryPool(device, queryPool, nullptr);

        destroySwapchain(swapchain);

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
//    if (pfnCmdDebugMarkerBegin)
//    {
//        VkDebugMarkerMarkerInfoEXT markerInfo = {};
//        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
//        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
//        markerInfo.pMarkerName = pMarkerName;
//        pfnCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
//    }
}

// Insert a new debug marker into the command buffer
void insertDebugRegion(VkCommandBuffer cmdbuffer, std::string_view markerName, Vec4 color)
{
    // Check for valid function pointer (may not be present if not running in a debugging application)
//    if (pfnCmdDebugMarkerInsert)
//    {
//        VkDebugMarkerMarkerInfoEXT markerInfo = {};
//        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
//        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
//        markerInfo.pMarkerName = markerName.c_str();
//        pfnCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
//    }
}

// End the current debug marker region
void endDebugRegion(VkCommandBuffer cmdBuffer)
{
    // Check for valid function (may not be present if not runnin in a debugging application)
//    if (pfnCmdDebugMarkerEnd)
//    {
//        pfnCmdDebugMarkerEnd(cmdBuffer);
//    }
}


void bindPipelineWithDecriptors(VkPipelineBindPoint bindPoint, const PipelineWithDescriptors &pipelineWithDescriptor)
{
    vkCmdBindPipeline(commandBuffer, bindPoint, pipelineWithDescriptor.pipeline.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, bindPoint, pipelineWithDescriptor.pipeline.pipelineLayout,
        0, 1, &pipelineWithDescriptor.descriptor.descriptorSet, 0, NULL);
}

Buffer &getRenderFrameBuffer()
{
    return renderFrameBuffer;
}

VkRenderPass getRenderPass()
{
    return renderPass;
}
