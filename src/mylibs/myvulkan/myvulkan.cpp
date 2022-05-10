#include "myvulkan.h"

#include <GLFW/glfw3.h>
#include "core/general.h"
#include "core/mytypes.h"

#include "math/vector3.h"

#include <vulkan/vulkan_core.h>

#include <string>
#include <set>
#include <string.h>

static constexpr uint32_t VulkanApiVersion = VK_API_VERSION_1_2;

static constexpr Formats defaultFormats[] = {
    { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    { VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_HDR10_ST2084_EXT },
};

static bool createGraphics();

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    PodVector<VkSurfaceFormatKHR> formats;
    PodVector<VkPresentModeKHR> presentModes;
};


// Intel?
//const VkFormat defaultFormat = VK_FORMAT_R8G8B8A8_UNORM;

VulkGlob vulk;


static PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag = nullptr;
static PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = nullptr;
static PFN_vkCmdDebugMarkerBeginEXT    pfnCmdDebugMarkerBegin = nullptr;
static PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd = nullptr;
static PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert = nullptr;











struct VulkanDeviceOptionals
{
    bool canUseVulkanRenderdocExtensionMarker = false;
};


static const PodVector<const char *>validationLayers =
{
    "VK_LAYER_KHRONOS_validation"
};


static const PodVector<const char *>deviceExtensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    //VK_KHR_MAINTENANCE1_EXTENSION_NAME
//    VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    bool errorMsg = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0 ||
        (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0;
    bool warningMsg = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0 ||
        (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0;

    if(!vulk.initParams.showInfoMessages && !errorMsg && !warningMsg)
        return VK_FALSE;

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
    if (!vulk.initParams.useValidationLayers)
        return nullptr;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugReportCallback;

    VkDebugUtilsMessengerEXT debugMessenger = 0;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    ASSERT(func);

    VK_CHECK(func(instance, &createInfo, nullptr, &debugMessenger));

    return debugMessenger;
}




static uint32_t selectMemoryType(const VkPhysicalDeviceMemoryProperties &memoryProperties, uint32_t memoryTypeBits, VkMemoryPropertyFlags flags)
{
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        bool isUsed = (memoryTypeBits & (1 << i)) != 0u;
        uint32_t foundFlags = (memoryProperties.memoryTypes[i].propertyFlags & flags);

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
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    PodVector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    VulkanDeviceOptionals result;

    for (const auto& extension : availableExtensions)
    {
        //printf("%s\n", extension.extensionName);
        if(vulk.initParams.useVulkanDebugMarkersRenderDoc && strcmp(extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0)
            result.canUseVulkanRenderdocExtensionMarker = true;
    }

    return result;
}


static PodVector<const char*> getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0u;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    PodVector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    //extensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);

    if (vulk.initParams.useValidationLayers)
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

    PodVector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = -1;
    for (const auto& queueFamily : queueFamilies)
    {
        ++i;
        uint32_t queueBits = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT;

        // make everything into same queue
        if ((queueFamily.queueFlags & queueBits) != queueBits)
            continue;
        indices.graphicsFamily = indices.transferFamily = indices.computeFamily = i;
        /*
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
        */
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

        if (!presentSupport)
            continue;

        indices.presentFamily = i;

        if (indices.isValid())
            break;
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
        vkDestroySwapchainKHR(vulk.device, swapchain.swapchain, nullptr);
    swapchain.swapchain = VK_NULL_HANDLE;
}




static bool createSwapchain(GLFWwindow *window, VSyncType vsyncMode)
{
    VkPresentModeKHR findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    switch (vsyncMode)
    {
        case VSyncType::FIFO_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR; break;
        case VSyncType::IMMEDIATE_NO_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR; break;
        case VSyncType::MAILBOX_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR; break;
    }
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulk.physicalDevice, vulk.surface);
        ASSERT(swapChainSupport.formats.size() > 0);
        VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
        bool found = false;
        for (const auto& availableFormat : swapChainSupport.formats)
        {
            if (availableFormat.format == vulk.colorFormat && availableFormat.colorSpace == vulk.colorSpace)
            {
                surfaceFormat = availableFormat;
                found = true;
                break;
            }
        }
        if(!found && swapChainSupport.formats.size() == 1 && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
        {
            surfaceFormat.colorSpace = vulk.colorSpace;
            surfaceFormat.format = vulk.colorFormat;
            found = true;
        }
        ASSERT(found);

        VkPresentModeKHR presentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& availablePresentMode : swapChainSupport.presentModes)
        {
            if (availablePresentMode == findPresentMode)
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

            extent.width = uint32_t(width);
            extent.height = uint32_t(height);

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
        createInfo.surface = vulk.surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.oldSwapchain = vulk.swapchain.swapchain;

        QueueFamilyIndices indices = findQueueFamilies(vulk.physicalDevice, vulk.surface);
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
        [[maybe_unused]] VkResult res = vkCreateSwapchainKHR(vulk.device, &createInfo, nullptr, &swapchain);
        if (res != VK_SUCCESS)
        {
            LOG("Failed to initialize swapchain\n");
            return false;
        }
        VK_CHECK(res);
        vulk.swapchain.swapchain = swapchain;

        vulk.colorSpace = surfaceFormat.colorSpace;
        vulk.colorFormat = surfaceFormat.format;
    }




    uint32_t swapchainCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(vulk.device, vulk.swapchain.swapchain, &swapchainCount, nullptr));

    vulk.swapchain.swapchainCount = swapchainCount;
    vulk.swapchain.images.resize(swapchainCount);
    VK_CHECK(vkGetSwapchainImagesKHR(vulk.device, vulk.swapchain.swapchain, &swapchainCount, vulk.swapchain.images.data()));

    int32_t width = 0;
    int32_t height = 0;

    glfwGetWindowSize(window, &width, &height);
    vulk.swapchain.width = uint32_t(width);
    vulk.swapchain.height = uint32_t(height);
    return true;
}










static bool createInstance()
{
    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VulkanApiVersion;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);


    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &appInfo;

    PodVector<const char*> extensions = getRequiredExtensions();

    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();

    for(auto ext : extensions)
    {
        printf("ext: %s\n", ext);
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    if (vulk.initParams.useValidationLayers)
    {
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.enabledLayerCount = uint32_t(validationLayers.size());

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

    vulk.instance = nullptr;
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vulk.instance));

    return vulk.instance != nullptr;
}





static bool createPhysicalDevice(VkPhysicalDeviceType wantedDeviceType)
{
    VkPhysicalDevice devices[256] = {};
    uint32_t count = ARRAYSIZES(devices);

    VK_CHECK(vkEnumeratePhysicalDevices(vulk.instance, &count, devices));

    VkPhysicalDevice primary = nullptr;
    VkPhysicalDevice secondary = nullptr;

    for(uint32_t i = 0; i < count; ++i)
    {
        VkPhysicalDeviceProperties prop;
        VkPhysicalDevice physicalDevice = devices[i];
        vkGetPhysicalDeviceProperties(physicalDevice, &prop);

        if(prop.apiVersion < VulkanApiVersion)
            continue;

        if(!prop.limits.timestampComputeAndGraphics)
            continue;

        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, vulk.surface);
        bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        if(!swapChainAdequate)
            continue;

        uint32_t formatIndex = ~0u;
        uint32_t optimalFormatFeatures = VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
        for (uint32_t j = 0; j < ARRAYSIZES(defaultFormats); ++j)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, defaultFormats[j].format, &formatProperties);
            if((formatProperties.optimalTilingFeatures & optimalFormatFeatures) == optimalFormatFeatures)
            {
                formatIndex = j;
                goto formatIndexFound;

            }
        }
        formatIndexFound:

        if(formatIndex == ~0u)
            continue;
        ASSERT(formatIndex != ~0u);
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, vulk.surface);
        if(!indices.isValid())
            continue;
        bool extensionsSupported = false;
        {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

            PodVector<VkExtensionProperties> availableExtensions(extensionCount);
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

        if(prop.deviceType == wantedDeviceType)
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

    vulk.physicalDevice = primary ? primary : secondary;

    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(vulk.physicalDevice, &prop);

    const char *typeText = prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated";
    printf("Picking %s device: %s\n", typeText, prop.deviceName);
    return true;
}



static bool createDeviceWithQueues()
{
    vulk.queueFamilyIndices = findQueueFamilies(vulk.physicalDevice, vulk.surface);
    ASSERT(vulk.queueFamilyIndices.isValid());

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulk.physicalDevice, vulk.surface);
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    if(!swapChainAdequate)
        return false;

    vulk.colorFormat = VK_FORMAT_UNDEFINED;
    vulk.depthFormat = defaultFormats[0].depth;
    vulk.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    vulk.computeColorFormat = VK_FORMAT_UNDEFINED;

    for(uint32_t i = 0; i < swapChainSupport.formats.size() && vulk.colorFormat == VK_FORMAT_UNDEFINED; ++i)
    {

        for (uint32_t j = 0; j < ARRAYSIZES(defaultFormats); ++j)
        {
            if(swapChainSupport.formats[i].colorSpace != defaultFormats[j].colorSpace)
                continue;
            if(swapChainSupport.formats[i].format != defaultFormats[j].format)
                continue;
            vulk.colorFormat = defaultFormats[j].format;
            vulk.depthFormat = defaultFormats[j].depth;

            vulk.colorSpace = defaultFormats[j].colorSpace;
        }
    }

    if(vulk.colorFormat == VK_FORMAT_UNDEFINED && swapChainSupport.formats.size() == 1 && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
    {
        vulk.colorFormat = defaultFormats[0].format;
        vulk.colorSpace = defaultFormats[0].colorSpace;
        vulk.depthFormat = defaultFormats[0].depth;
    }
    ASSERT(vulk.colorFormat != VK_FORMAT_UNDEFINED);


    for (uint32_t j = 0; j < ARRAYSIZES(defaultFormats); ++j)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(vulk.physicalDevice, defaultFormats[j].format, &formatProperties);
        if((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0u &&
             (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0u)
        {
            vulk.computeColorFormat = defaultFormats[j].format;
            break;
        }
    }

    ASSERT(vulk.computeColorFormat != VK_FORMAT_UNDEFINED);

    PodVector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        vulk.queueFamilyIndices.graphicsFamily, vulk.queueFamilyIndices.presentFamily
    };

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


    PodVector<const char *> deviceExts = deviceExtensions;

    VulkanDeviceOptionals optionals = getDeviceOptionals(vulk.physicalDevice);
    if(optionals.canUseVulkanRenderdocExtensionMarker)
    {
        deviceExts.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    }
    createInfo.enabledExtensionCount = uint32_t(deviceExts.size());
    createInfo.ppEnabledExtensionNames = deviceExts.data();


    if (vulk.initParams.useValidationLayers)
    {
        createInfo.enabledLayerCount = uint32_t(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK(vkCreateDevice(vulk.physicalDevice, &createInfo, nullptr, &vulk.device));
    ASSERT(vulk.device);

    vkGetDeviceQueue(vulk.device, vulk.queueFamilyIndices.graphicsFamily, 0, &vulk.graphicsQueue);
    vkGetDeviceQueue(vulk.device, vulk.queueFamilyIndices.presentFamily, 0, &vulk.presentQueue);
    vkGetDeviceQueue(vulk.device, vulk.queueFamilyIndices.computeFamily, 0, &vulk.computeQueue);
    ASSERT(vulk.graphicsQueue);
    ASSERT(vulk.presentQueue);
    ASSERT(vulk.computeQueue);

    if(optionals.canUseVulkanRenderdocExtensionMarker)
        acquireDeviceDebugRenderdocFunctions(vulk.device);
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

    if (vulk.swapchain.width == newWidth && vulk.swapchain.height == newHeight)
        return false;

    VK_CHECK(vkDeviceWaitIdle(vulk.device));

    SwapChain oldSwapchain = vulk.swapchain;
    createSwapchain(window, vulk.initParams.vsync);

    destroySwapchain(oldSwapchain);
    return true;
}






static VkSemaphore createSemaphore()
{
    VkSemaphore semaphore = 0;
    VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VK_CHECK(vkCreateSemaphore(vulk.device, &semaphoreInfo, nullptr, &semaphore));
    return semaphore;
}

static VkCommandPool createCommandPool()
{
    uint32_t familyIndex = vulk.queueFamilyIndices.graphicsFamily;
    VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolCreateInfo.queueFamilyIndex = familyIndex;

    VkCommandPool commandPool = 0;
    VK_CHECK(vkCreateCommandPool(vulk.device, &poolCreateInfo, nullptr, &commandPool));

    return commandPool;
}


static VkRenderPass createRenderPass(VkFormat colorFormat, VkFormat depthFormat)
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
    createInfo.attachmentCount = ARRAYSIZES(attachments);
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;

    VkRenderPass renderPass = 0;

    VK_CHECK(vkCreateRenderPass(vulk.device, &createInfo, nullptr, &renderPass));
    return renderPass;
}



static VkQueryPool createQueryPool(uint32_t queryCount)
{
    VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };

    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = queryCount;

    VkQueryPool pool = 0;
    VK_CHECK(vkCreateQueryPool(vulk.device, &createInfo, nullptr, &pool));

    ASSERT(pool);
    return pool;
}


static VkFence createFence()
{
    VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence result = 0;
    VK_CHECK(vkCreateFence(vulk.device, &createInfo, nullptr, &result));
    ASSERT(result);
    return result;
}




















bool initVulkan(GLFWwindow *window, const VulkanInitializationParameters &initParameters)
{
    vulk.initParams = initParameters;
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

    vulk.debugCallBack = registerDebugCallback(vulk.instance);

    VK_CHECK(glfwCreateWindowSurface(vulk.instance, window, nullptr, &vulk.surface));
    ASSERT(vulk.surface);
    if(!vulk.surface)
    {
        printf("Failed to create vulkan surface!\n");
        return false;
    }


    if(!createPhysicalDevice(vulk.initParams.useIntegratedGpu ?
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU : VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
    {
        printf("Failed to create vulkan physical device!\n");
        return false;
    }
    ASSERT(vulk.physicalDevice);


    if(!createDeviceWithQueues())
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

        vkGetPhysicalDeviceProperties2(vulk.physicalDevice, &physicalDeviceProperties);

        printf("subgroup size: %u\n", subgroupProperties.subgroupSize);
        printf("subgroup operations: %u\n", subgroupProperties.supportedOperations);

    }
    vulk.renderPass = createRenderPass(vulk.computeColorFormat, vulk.depthFormat);
    ASSERT(vulk.renderPass);
    if(!vulk.renderPass)
    {
        printf("Failed to create render pass!\n");
        return false;
    }

    bool scSuccess = createSwapchain(window, vulk.initParams.vsync);
    ASSERT(scSuccess);
    if(!scSuccess)
    {
        printf("Failed to create vulkan swapchain!\n");
        return false;
    }

    vulk.queryPool = createQueryPool(QUERY_COUNT);
    ASSERT(vulk.queryPool);
    if(!vulk.queryPool)
    {
        printf("Failed to create vulkan query pool!\n");
        return false;
    }

    vulk.acquireSemaphore = createSemaphore();
    ASSERT(vulk.acquireSemaphore);
    if(!vulk.acquireSemaphore)
    {
        printf("Failed to create vulkan acquire semapohore!\n");
        return false;
    }

    vulk.releaseSemaphore = createSemaphore();
    ASSERT(vulk.releaseSemaphore);
    if(!vulk.releaseSemaphore)
    {
        printf("Failed to create vulkan release semaphore!\n");
        return false;
    }

    vulk.fence = createFence();
    ASSERT(vulk.fence);
    if(!vulk.fence)
    {
        printf("Failed to create vulkan fence!\n");
        return false;
    }

    vulk.commandPool = createCommandPool();
    ASSERT(vulk.commandPool);
    if(!vulk.commandPool)
    {
        printf("Failed to create vulkan command pool!\n");
        return false;
    }

    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = vulk.commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VK_CHECK(vkAllocateCommandBuffers(vulk.device, &allocateInfo, &vulk.commandBuffer));
    if(!vulk.commandBuffer)
    {
        printf("Failed to create vulkan command buffer!\n");
        return false;
    }
    setObjectName((uint64_t)vulk.commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "Main command buffer");


    {
        // Create buffers
        vkGetPhysicalDeviceMemoryProperties(vulk.physicalDevice, &vulk.memoryProperties);

        vulk.scratchBuffer = createBuffer(64 * 1024 * 1024,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Scratch buffer");

        vulk.renderFrameBuffer = createBuffer(64u * 1024 * 1024,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Frame render uniform buffer");

    }
    createGraphics();

    return true;
}

static void deleteFrameTargets()
{
    vkDestroyFramebuffer(vulk.device, vulk.targetFB, nullptr);
    vulk.targetFB = nullptr;
    destroyImage(vulk.mainColorRenderTarget);
    destroyImage(vulk.mainDepthRenderTarget);
}

static bool createGraphics()
{
    // create color and depth images
    vulk.mainColorRenderTarget = createImage(
        vulk.swapchain.width, vulk.swapchain.height,
        //deviceWithQueues.computeColorFormat,
        vulk.colorFormat,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main color target image");

    vulk.mainDepthRenderTarget = createImage(
        vulk.swapchain.width, vulk.swapchain.height, vulk.depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Main depth target image");

    vulk.targetFB = createFramebuffer(vulk.renderPass,
        vulk.mainColorRenderTarget.imageView, vulk.mainDepthRenderTarget.imageView,
        vulk.swapchain.width, vulk.swapchain.height);

    return true;
}







static VkDescriptorSetLayout createSetLayout(const PodVector<DescriptorSetLayout> &descriptors, VkShaderStageFlags stage)
{
    ASSERT(descriptors.size() > 0);
    PodVector<VkDescriptorSetLayoutBinding> setBindings(descriptors.size());
    for(uint32_t i = 0; i < uint32_t(descriptors.size()); ++i)
    {
        setBindings[i].binding = descriptors[i].bindingIndex;
        setBindings[i].descriptorType = descriptors[i].descriptorType; // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        setBindings[i].descriptorCount = 1;
        setBindings[i].stageFlags = stage; // VK_SHADER_STAGE_VERTEX_BIT;
    }

    VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
//    createInfo.flags =  VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    createInfo.bindingCount = uint32_t(setBindings.size());
    createInfo.pBindings = setBindings.data();

    VkDescriptorSetLayout setLayout = 0;
    VK_CHECK(vkCreateDescriptorSetLayout(vulk.device, &createInfo, nullptr, &setLayout));
    ASSERT(setLayout);
    return setLayout;
}

bool createPipelineLayout(PipelineWithDescriptors& pipelineWithDescriptors, VkShaderStageFlags stage)
{
    VkDescriptorSetLayout setLayout = 0;
    VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

    if(pipelineWithDescriptors.descriptorSetLayouts.size() > 0)
        setLayout = createSetLayout(pipelineWithDescriptors.descriptorSetLayouts, stage);


    createInfo.setLayoutCount = setLayout ? 1 : 0;
    createInfo.pSetLayouts = setLayout ? &setLayout : nullptr;

    VkPipelineLayout layout = 0;
    VK_CHECK(vkCreatePipelineLayout(vulk.device, &createInfo, nullptr, &layout));
    ASSERT(layout);
    if (!layout)
        return false;

    pipelineWithDescriptors.pipelineLayout = layout;
    pipelineWithDescriptors.descriptorSetLayout = setLayout;
    return true;
}

static void recreateSwapchainData()
{
    deleteFrameTargets();
    createGraphics();
    vulk.needToResize = false;
}



















void beginSingleTimeCommands()
{
    VK_CHECK(vkResetCommandPool(vulk.device, vulk.commandPool, 0));

    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(vulk.commandBuffer, &beginInfo));

    return;
}

void endSingleTimeCommands()
{
    VK_CHECK(vkEndCommandBuffer(vulk.commandBuffer));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulk.commandBuffer;

    VkQueue queue = vulk.graphicsQueue;
    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(queue));
}








Image createImage(uint32_t width, uint32_t height, VkFormat format,
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
    createInfo.pQueueFamilyIndices = &vulk.queueFamilyIndices.graphicsFamily;
    Image result;

    VK_CHECK(vkCreateImage(vulk.device, &createInfo, nullptr, &result.image));
    ASSERT(result.image);

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(vulk.device, result.image, &memoryRequirements);

    uint32_t memoryTypeIndex = selectMemoryType(vulk.memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags);
    ASSERT(memoryTypeIndex != ~0u);

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    VK_CHECK(vkAllocateMemory(vulk.device, &allocateInfo, nullptr, &result.deviceMemory));
    ASSERT(result.deviceMemory);

    VK_CHECK(vkBindImageMemory(vulk.device, result.image, result.deviceMemory, 0));

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
    setObjectName((uint64_t)result.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, imageName);
    result.imageName = imageName;

    return result;
}



void updateImageWithData(uint32_t width, uint32_t height, uint32_t pixelSize,
    Image &targetImage,
    uint32_t dataSize, void *data)
{
    ASSERT(data != nullptr || dataSize > 0u);
    ASSERT(vulk.scratchBuffer.data);
    ASSERT(vulk.scratchBuffer.size >= width * height * pixelSize);
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

        vkCmdPipelineBarrier(vulk.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZES(imageBarriers), imageBarriers);
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

    vkCmdCopyBufferToImage(vulk.commandBuffer, vulk.scratchBuffer.buffer, targetImage.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    {
        VkImageMemoryBarrier imageBarriers[] =
        {
            imageBarrier(targetImage,
                        VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL),
        };

        vkCmdPipelineBarrier(vulk.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZES(imageBarriers), imageBarriers);
    }
    endSingleTimeCommands();

    return;
}

VkSampler createSampler(const VkSamplerCreateInfo &info)
{
    VkSampler sampler = nullptr;
    VK_CHECK(vkCreateSampler(vulk.device, &info, nullptr, &sampler));
    return sampler;
}

void destroyImage(Image &image)
{
    vkDestroyImageView(vulk.device, image.imageView, nullptr);
    vkDestroyImage(vulk.device, image.image, nullptr);
    vkFreeMemory(vulk.device, image.deviceMemory, nullptr);

    image = Image{};
}


void destroySampler(VkSampler sampler)
{
    vkDestroySampler(vulk.device, sampler, nullptr);
}

void destroyShaderModule(VkShaderModule shaderModule)
{
    vkDestroyShaderModule(vulk.device, shaderModule, nullptr);
}


Buffer createBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, const char *bufferName)
{
    Buffer result;

    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = size;
    createInfo.usage = usage;

    VK_CHECK(vkCreateBuffer(vulk.device, &createInfo, nullptr, &result.buffer));
    ASSERT(result.buffer);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(vulk.device, result.buffer, &memoryRequirements);

    uint32_t memoryTypeIndex = selectMemoryType(vulk.memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags);
    ASSERT(memoryTypeIndex != ~0u);

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    VK_CHECK(vkAllocateMemory(vulk.device, &allocateInfo, nullptr, &result.deviceMemory));
    ASSERT(result.deviceMemory);

    VK_CHECK(vkBindBufferMemory(vulk.device, result.buffer, result.deviceMemory, 0));

    void *data = nullptr;
    if(memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        VK_CHECK(vkMapMemory(vulk.device, result.deviceMemory, 0, size, 0, &data));
        ASSERT(data);
    }
    result.data = data;
    result.size = size;


    setObjectName((uint64_t)result.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, bufferName);
    result.bufferName = bufferName;

    return result;
}

size_t uploadToScratchbuffer(void* data, size_t size, size_t offset)
{
    ASSERT(vulk.scratchBuffer.data);
    ASSERT(size);
    ASSERT(vulk.scratchBuffer.size >= size);

    memcpy((unsigned char*)vulk.scratchBuffer.data + offset, data, size);
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
    ASSERT(vulk.scratchBuffer.data);
    ASSERT(vulk.scratchBuffer.size >= size);
    ASSERT(gpuBuffer.size >= size);

    beginSingleTimeCommands();

    VkBufferCopy region = { 0, 0, VkDeviceSize(size) };
    vkCmdCopyBuffer(vulk.commandBuffer, vulk.scratchBuffer.buffer, gpuBuffer.buffer, 1, &region);

    VkBufferMemoryBarrier copyBarrier = bufferBarrier(gpuBuffer.buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, size);
    vkCmdPipelineBarrier(vulk.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &copyBarrier, 0, nullptr);

    endSingleTimeCommands();

}

void destroyBuffer(Buffer &buffer)
{
    vkDestroyBuffer(vulk.device, buffer.buffer, nullptr);
    vkFreeMemory(vulk.device, buffer.deviceMemory, nullptr);
    buffer.buffer = 0;
    buffer.data = nullptr;
    buffer.deviceMemory = 0;
    buffer.size = 0ll;
}


bool startRender(GLFWwindow *window)
{
    VK_CHECK(vkWaitForFences(vulk.device, 1, &vulk.fence, VK_TRUE, UINT64_MAX));
    if (vulk.acquireSemaphore == VK_NULL_HANDLE)
        return false;
    VkResult res = ( vkAcquireNextImageKHR(vulk.device, vulk.swapchain.swapchain, UINT64_MAX,
        vulk.acquireSemaphore, VK_NULL_HANDLE, &vulk.imageIndex) );
    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if (resizeSwapchain(window))
        {
            recreateSwapchainData();
            VK_CHECK(vkDeviceWaitIdle(vulk.device));
            vulk.needToResize = false;
        }
        return false;
    }
    else if (res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR)
        return true;

    VK_CHECK(res);
    return false;
}

void present(GLFWwindow *window)
{
    Image &presentImage = vulk.mainColorRenderTarget;
    // Copy final image to swap chain target
    {
        VkImageMemoryBarrier copyBeginBarriers[] =
        {
            imageBarrier(presentImage,
                VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),

            imageBarrier(vulk.swapchain.images[ vulk.imageIndex ],
                0, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        };

        vkCmdPipelineBarrier(vulk.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZES(copyBeginBarriers), copyBeginBarriers);


        insertDebugRegion("Copy to swapchain", Vec4(1.0f, 1.0f, 0.0f, 1.0f));

        VkImageBlit imageBlitRegion = {};

        imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlitRegion.srcSubresource.layerCount = 1;
        imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlitRegion.dstSubresource.layerCount = 1;
        imageBlitRegion.srcOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
        imageBlitRegion.srcOffsets[ 1 ] = VkOffset3D{ ( int32_t ) vulk.swapchain.width, ( int32_t ) vulk.swapchain.height, 1 };
        imageBlitRegion.dstOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
        imageBlitRegion.dstOffsets[ 1 ] = VkOffset3D{ ( int32_t ) vulk.swapchain.width, ( int32_t ) vulk.swapchain.height, 1 };


        vkCmdBlitImage(vulk.commandBuffer,
            presentImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vulk.swapchain.images[ vulk.imageIndex ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VkFilter::VK_FILTER_NEAREST);
    }

    // Prepare image for presenting.
    {
        VkImageMemoryBarrier presentBarrier =
            imageBarrier(vulk.swapchain.images[ vulk.imageIndex ],
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vkCmdPipelineBarrier(vulk.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &presentBarrier);
    }


    endDebugRegion();

    VK_CHECK(vkEndCommandBuffer(vulk.commandBuffer));

    // Submit
    {
        VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; //VK_PIPELINE_STAGE_TRANSFER_BIT;

        vkResetFences(vulk.device, 1, &vulk.fence);

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &vulk.acquireSemaphore;
        submitInfo.pWaitDstStageMask = &submitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vulk.commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &vulk.releaseSemaphore;
        VK_CHECK(vkQueueSubmit(vulk.graphicsQueue, 1, &submitInfo, vulk.fence));

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &vulk.releaseSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vulk.swapchain.swapchain;
        presentInfo.pImageIndices = &vulk.imageIndex;

        VkResult res = ( vkQueuePresentKHR(vulk.presentQueue, &presentInfo) );
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || vulk.needToResize)
        {
            vulk.needToResize = true;
            if (resizeSwapchain(window))
                recreateSwapchainData();

            vulk.needToResize = false;
        }
        else
        {
            VK_CHECK(res);
        }
    }

    VK_CHECK(vkDeviceWaitIdle(vulk.device));

}


VkFramebuffer createFramebuffer(VkRenderPass renderPass,
    VkImageView colorView, VkImageView depthView, uint32_t width, uint32_t height)
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
    VK_CHECK(vkCreateFramebuffer(vulk.device, &createInfo, nullptr, &framebuffer));
    return framebuffer;
}

VkImageView createImageView(VkImage image, VkFormat format)
{
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if(format == VK_FORMAT_D32_SFLOAT ||
        format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT ||
        format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
        format == VK_FORMAT_D16_UNORM ||
        format == VK_FORMAT_D16_UNORM_S8_UINT ||
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
    VK_CHECK(vkCreateImageView(vulk.device, &createInfo, nullptr, &view));

    ASSERT(view);
    return view;
}


VkImageMemoryBarrier imageBarrier(Image& image,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask)
{

    VkImageMemoryBarrier barrier =
        imageBarrier(image.image, image.accessMask, image.layout, dstAccessMask, newLayout, aspectMask);
    image.accessMask = dstAccessMask;
    image.layout = newLayout;
    return barrier;
}

VkImageMemoryBarrier imageBarrier(Image& image,
    VkAccessFlags srcAccessMask, VkImageLayout oldLayout,
    VkAccessFlags dstAccessMask, VkImageLayout newLayout,
    VkImageAspectFlags aspectMask)
{

    VkImageMemoryBarrier barrier =
        imageBarrier(image.image, srcAccessMask, oldLayout, dstAccessMask, newLayout, aspectMask);
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
























Descriptor createDescriptor(const PodVector<DescriptorSetLayout> &descriptors, VkDescriptorSetLayout descriptorSetLayout)
{
    Descriptor result;
    if(descriptors.size() == 0)
        return result;

    PodVector<VkDescriptorPoolSize> poolSizes(descriptors.size());

    for(uint32_t i = 0; i < descriptors.size(); ++i)
    {
        poolSizes[i].type = descriptors[i].descriptorType;
        poolSizes[i].descriptorCount = 1;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = uint32_t(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1; // NOTE ????

    VK_CHECK(vkCreateDescriptorPool(vulk.device, &poolInfo, nullptr, &result.pool));

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = result.pool;
    allocInfo.descriptorSetCount = 1u;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet = 0;
    VK_CHECK(vkAllocateDescriptorSets(vulk.device, &allocInfo, &descriptorSet));
    ASSERT(descriptorSet);

    result.descriptorSet = descriptorSet;
    return result;
}

bool setBindDescriptorSet(const PodVector<DescriptorSetLayout> &descriptors,
    const PodVector<DescriptorInfo> &descriptorInfos, VkDescriptorSet descriptorSet)
{
    uint32_t writeDescriptorCount = (uint32_t)descriptorInfos.size();
    if(writeDescriptorCount < 1u)
        return false;

    PodVector<VkWriteDescriptorSet> writeDescriptorSets(writeDescriptorCount);
    PodVector<VkDescriptorBufferInfo> bufferInfos(writeDescriptorCount);

    PodVector<VkDescriptorImageInfo> imageInfos(writeDescriptorCount);

    uint32_t writeIndex = 0u;
    uint32_t bufferCount = 0u;
    uint32_t imageCount = 0u;
    for(uint32_t i = 0; i < descriptorInfos.size(); ++i)
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
        vkUpdateDescriptorSets(vulk.device, uint32_t(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
    return true;
}


void destroyDescriptor(Descriptor &descriptor)
{
    vkDestroyDescriptorPool(vulk.device, descriptor.pool, nullptr);
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
    dynamicInfo.dynamicStateCount = ARRAYSIZES(dynamicStates);

    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    createInfo.stageCount = ARRAYSIZES(stageInfos);
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
    VK_CHECK(vkCreateGraphicsPipelines(vulk.device, nullptr, 1, &createInfo, nullptr, &pipeline));
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
    VK_CHECK(vkCreateComputePipelines(vulk.device, nullptr, 1, &createInfo, nullptr, &pipeline));
    ASSERT(pipeline);

    return pipeline;
}


void destroyPipeline(PipelineWithDescriptors &pipeline)
{
    if(pipeline.pipeline)
        vkDestroyPipeline(vulk.device, pipeline.pipeline, nullptr);
    if(pipeline.pipelineLayout)
        vkDestroyPipelineLayout(vulk.device, pipeline.pipelineLayout, nullptr );

    if(pipeline.descriptorSetLayout)
        vkDestroyDescriptorSetLayout(vulk.device, pipeline.descriptorSetLayout, nullptr);

    pipeline.pipeline = nullptr;
    pipeline.pipelineLayout = nullptr;
    pipeline.descriptorSetLayout = nullptr;
}






VkShaderModule loadShader(std::string_view filename)
{
    PodVector<char> buffer;
    VkShaderModule shaderModule = 0;

    if (loadBytes(filename, buffer))
    {
        ASSERT(buffer.size() % 4 == 0);

        VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = buffer.size();
        createInfo.pCode = reinterpret_cast<uint32_t*>(buffer.data());
        vkCreateShaderModule(vulk.device, &createInfo, nullptr, &shaderModule);
    }
    ASSERT(shaderModule);
    return shaderModule;
}



















void deinitVulkan()
{
    if(vulk.device)
    {
        destroyBuffer(vulk.scratchBuffer);
        destroyBuffer(vulk.renderFrameBuffer);

        deleteFrameTargets();

        vkDestroyCommandPool(vulk.device, vulk.commandPool, nullptr);

        vkDestroyQueryPool(vulk.device, vulk.queryPool, nullptr);

        destroySwapchain(vulk.swapchain);

        vkDestroyRenderPass(vulk.device, vulk.renderPass, nullptr);
        vkDestroyFence(vulk.device, vulk.fence, nullptr);
        vkDestroySemaphore(vulk.device, vulk.acquireSemaphore, nullptr);
        vkDestroySemaphore(vulk.device, vulk.releaseSemaphore, nullptr);

        vkDestroyDevice(vulk.device, nullptr);
    }
    vkDestroySurfaceKHR(vulk.instance, vulk.surface, nullptr);

    if (vulk.initParams.useValidationLayers)
    {
        auto dest = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulk.instance, "vkDestroyDebugUtilsMessengerEXT");
        dest(vulk.instance, vulk.debugCallBack, nullptr);
    }
    vkDestroyInstance(vulk.instance, nullptr);

}


// From sasha wilems debugmarker
void setObjectName(uint64_t object, VkDebugReportObjectTypeEXT objectType, std::string_view name)
{
    // Check for a valid function pointer
    if (pfnDebugMarkerSetObjectName)
    {
        VkDebugMarkerObjectNameInfoEXT nameInfo = {};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.object = object;
        nameInfo.pObjectName = name.data();
        pfnDebugMarkerSetObjectName(vulk.device, &nameInfo);
    }
}

void setObjectTag(uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag)
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
        pfnDebugMarkerSetObjectTag(vulk.device, &tagInfo);
    }
}

// Start a new debug marker region
void beginDebugRegion(std::string_view pMarkerName, Vec4 color)
{
    // Check for valid function pointer (may not be present if not running in a debugging application)
//    if (pfnCmdDebugMarkerBegin)
//    {
//        VkDebugMarkerMarkerInfoEXT markerInfo = {};
//        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
//        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
//        markerInfo.pMarkerName = pMarkerName;
//        pfnCmdDebugMarkerBegin(vulk.commandBuffer, &markerInfo);
//    }
}

// Insert a new debug marker into the command buffer
void insertDebugRegion(std::string_view markerName, Vec4 color)
{
    // Check for valid function pointer (may not be present if not running in a debugging application)
//    if (pfnCmdDebugMarkerInsert)
//    {
//        VkDebugMarkerMarkerInfoEXT markerInfo = {};
//        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
//        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
//        markerInfo.pMarkerName = markerName.c_str();
//        pfnCmdDebugMarkerInsert(vulk.commandBuffer, &markerInfo);
//    }
}

// End the current debug marker region
void endDebugRegion()
{
    // Check for valid function (may not be present if not runnin in a debugging application)
//    if (pfnCmdDebugMarkerEnd)
//    {
//        pfnCmdDebugMarkerEnd(vulk.commandBuffer);
//    }
}


void bindPipelineWithDecriptors(VkPipelineBindPoint bindPoint, const PipelineWithDescriptors &pipelineWithDescriptor)
{
    vkCmdBindPipeline(vulk.commandBuffer, bindPoint, pipelineWithDescriptor.pipeline);
    vkCmdBindDescriptorSets(vulk.commandBuffer, bindPoint, pipelineWithDescriptor.pipelineLayout,
        0, 1, &pipelineWithDescriptor.descriptor.descriptorSet, 0, NULL);
}

