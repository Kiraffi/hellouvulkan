
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "myvulkan.h"

#include <container/podvector.h>
#include <core/general.h>
#include <core/mytypes.h>
#include <core/vulkan_app.h>
#include <math/vector3.h>
#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

#include <vulkan/vulkan_core.h>


#include <vk_mem_alloc.h>

#include <string_view>
#include <set>
#include <string.h>


static constexpr uint32_t FormatFlagBits =
    VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
    VK_FORMAT_FEATURE_BLIT_SRC_BIT |
    VK_FORMAT_FEATURE_BLIT_DST_BIT |
    VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
    VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
    VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

static constexpr Formats defaultPresent[] = {
    { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    { VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_HDR10_ST2084_EXT },
};

static constexpr Formats defaultFormats[] = {
    { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
    { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
};


struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    PodVector<VkSurfaceFormatKHR> formats;
    PodVector<VkPresentModeKHR> presentModes;
};


// Intel?
//const VkFormat defaultFormat = VK_FORMAT_R8G8B8A8_UNORM;

VulkanGlobal*vulk = nullptr;


static PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag = nullptr;
static PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = nullptr;
static PFN_vkCmdDebugMarkerBeginEXT    pfnCmdDebugMarkerBegin = nullptr;
static PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd = nullptr;
static PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert = nullptr;











struct VulkanDeviceOptionals
{
    bool canUseVulkanRenderdocExtensionMarker = false;
};

static const VkValidationFeatureEnableEXT enabledValidationFeatures[] =
{
    //VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
    //VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
    //VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    //VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
};
static const char *validationLayers[] =
{
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_monitor",
    "VK_LAYER_KHRONOS_synchronization2",


    //"VK_LAYER_NV_optimus",
    //"VK_LAYER_RENDERDOC_Capture",
    //"VK_LAYER_KHRONOS_profiles",
    //"VK_LAYER_LUNARG_screenshot",
    //"VK_LAYER_LUNARG_gfxreconstruct",
    //"VK_LAYER_LUNARG_api_dump"
};


static const char * deviceExtensions[] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    // VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    // VK_KHR_MAINTENANCE1_EXTENSION_NAME
    // VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
};

static const char* addCheckDeviceExtensions[] =
{
    // if this extension is put into device extensions, it throws some validation errors,
    // warning about it being incomplete?
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
};

static PodVector<const char*> getRequiredInstanceExtensions()
{
    uint32_t glfwExtensionCount = 0u;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    PodVector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    extensions.pushBack(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    if (vulk->initParams.useValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        //extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
    return extensions;
}

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

    if(!vulk->initParams.showInfoMessages && !errorMsg && !warningMsg)
        return VK_FALSE;

    const char *type = errorMsg ? "Error" : (warningMsg  ? "Warning" : "Info");

    printf("Type:%s, message: %s\n\n", type, pCallbackData->pMessage);
    if(errorMsg)
    {
        ASSERT(!"Validation error encountered!");
    }
    return VK_FALSE;
}

static VkDebugUtilsMessengerEXT registerDebugCallback(VkInstance instance)
{
    if (!vulk->initParams.useValidationLayers)
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
        if(vulk->initParams.useVulkanDebugMarkersRenderDoc && strcmp(extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0)
            result.canUseVulkanRenderdocExtensionMarker = true;
    }

    return result;
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
        vkDestroySwapchainKHR(vulk->device, swapchain.swapchain, nullptr);
    swapchain.swapchain = VK_NULL_HANDLE;
}




static bool createSwapchain(VSyncType vsyncMode)
{
    VkPresentModeKHR findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    switch (vsyncMode)
    {
        case VSyncType::FIFO_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR; break;
        case VSyncType::IMMEDIATE_NO_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_IMMEDIATE_KHR; break;
        case VSyncType::MAILBOX_VSYNC: findPresentMode = VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR; break;
    }
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulk->physicalDevice, vulk->surface);
        ASSERT(swapChainSupport.formats.size() > 0);
        VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
        bool found = false;
        for (const auto& availableFormat : swapChainSupport.formats)
        {
            if (availableFormat.format == vulk->presentColorFormat && availableFormat.colorSpace == vulk->colorSpace)
            {
                surfaceFormat = availableFormat;
                found = true;
                break;
            }
        }
        if(!found && swapChainSupport.formats.size() == 1 && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
        {
            surfaceFormat.colorSpace = vulk->colorSpace;
            surfaceFormat.format = vulk->presentColorFormat;
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
            glfwGetFramebufferSize(vulk->vulkanApp->window, &width, &height);

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
        createInfo.surface = vulk->surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;// | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.oldSwapchain = vulk->swapchain.swapchain;

        QueueFamilyIndices indices = findQueueFamilies(vulk->physicalDevice, vulk->surface);
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
        VkResult res = vkCreateSwapchainKHR(vulk->device, &createInfo, nullptr, &swapchain);
        VK_CHECK(res);
        if (res != VK_SUCCESS)
        {
            LOG("Failed to initialize swapchain\n");
            return false;
        }
        vulk->swapchain.swapchain = swapchain;

        vulk->colorSpace = surfaceFormat.colorSpace;
        vulk->presentColorFormat = surfaceFormat.format;
    }




    uint32_t swapchainCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(vulk->device, vulk->swapchain.swapchain, &swapchainCount, nullptr));

    vulk->swapchain.swapchainCount = swapchainCount;
    vulk->swapchain.images.resize(swapchainCount);
    VK_CHECK(vkGetSwapchainImagesKHR(vulk->device, vulk->swapchain.swapchain, &swapchainCount, vulk->swapchain.images.data()));

    int32_t width = 0;
    int32_t height = 0;

    glfwGetWindowSize(vulk->vulkanApp->window, &width, &height);
    vulk->swapchain.width = uint32_t(width);
    vulk->swapchain.height = uint32_t(height);
    return true;
}










static bool createInstance()
{
    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.apiVersion = VulkanApiVersion;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);


    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &appInfo;

    PodVector<const char*> extensions = getRequiredInstanceExtensions();

    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();

    for(auto ext : extensions)
    {
        printf("instance ext: %s\n", ext);
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    if (vulk->initParams.useValidationLayers)
    {
        createInfo.ppEnabledLayerNames = validationLayers;
        createInfo.enabledLayerCount = ARRAYSIZES(validationLayers);

        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugReportCallback;


        VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
        validationFeatures.enabledValidationFeatureCount = ARRAYSIZES(enabledValidationFeatures);
        validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

        debugCreateInfo.pNext = &validationFeatures;

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    vulk->instance = nullptr;
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &vulk->instance));

    return vulk->instance != nullptr;
}





static bool createPhysicalDevice(VkPhysicalDeviceType wantedDeviceType)
{
    VkPhysicalDevice devices[256] = {};
    uint32_t count = ARRAYSIZES(devices);

    VK_CHECK(vkEnumeratePhysicalDevices(vulk->instance, &count, devices));

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

        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, vulk->surface);
        bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        if(!swapChainAdequate)
            continue;

        uint32_t formatIndex = ~0u;

        for (uint32_t j = 0; j < ARRAYSIZES(defaultFormats); ++j)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, defaultFormats[j].format, &formatProperties);
            if(((formatProperties.optimalTilingFeatures) & FormatFlagBits) == FormatFlagBits)
            {
                formatIndex = j;
                goto formatIndexFound;

            }
        }
        formatIndexFound:

        if(formatIndex == ~0u)
            continue;
        ASSERT(formatIndex != ~0u);
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, vulk->surface);
        if(!indices.isValid())
            continue;
        bool extensionsSupported = false;
        {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

            PodVector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string_view> requiredExtensions;
            for (const char *str : deviceExtensions)
                requiredExtensions.insert(str);

            for(const char *str : addCheckDeviceExtensions)
                requiredExtensions.insert(str);

            for (const auto& extension : availableExtensions)
            {
                //printf("available extension: %s\n", extension.extensionName);
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

    vulk->physicalDevice = primary ? primary : secondary;

    VkPhysicalDeviceProperties prop;
    vkGetPhysicalDeviceProperties(vulk->physicalDevice, &prop);

    const char *typeText = prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "discrete" : "integrated";
    printf("Picking %s device: %s\n", typeText, prop.deviceName);
    return true;
}



static bool createDeviceWithQueues()
{
    vulk->queueFamilyIndices = findQueueFamilies(vulk->physicalDevice, vulk->surface);
    ASSERT(vulk->queueFamilyIndices.isValid());

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vulk->physicalDevice, vulk->surface);
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    if(!swapChainAdequate)
        return false;

    vulk->presentColorFormat = VK_FORMAT_UNDEFINED;
    vulk->depthFormat = defaultPresent[0].depth;
    vulk->colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    vulk->defaultColorFormat = VK_FORMAT_UNDEFINED;

    for(uint32_t i = 0; i < swapChainSupport.formats.size() && vulk->presentColorFormat == VK_FORMAT_UNDEFINED; ++i)
    {

        for (uint32_t j = 0; j < ARRAYSIZES(defaultPresent); ++j)
        {
            if(swapChainSupport.formats[i].colorSpace != defaultPresent[j].colorSpace)
                continue;
            if(swapChainSupport.formats[i].format != defaultPresent[j].format)
                continue;
            vulk->presentColorFormat = defaultPresent[j].format;
            vulk->depthFormat = defaultPresent[j].depth;

            vulk->colorSpace = defaultPresent[j].colorSpace;
        }
    }

    if(vulk->presentColorFormat == VK_FORMAT_UNDEFINED && swapChainSupport.formats.size() == 1 && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
    {
        vulk->presentColorFormat = defaultPresent[0].format;
        vulk->colorSpace = defaultPresent[0].colorSpace;
        vulk->depthFormat = defaultPresent[0].depth;
    }
    ASSERT(vulk->presentColorFormat != VK_FORMAT_UNDEFINED);

    for (uint32_t j = 0; j < ARRAYSIZES(defaultFormats); ++j)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(vulk->physicalDevice, defaultFormats[j].format, &formatProperties);
        if (((formatProperties.optimalTilingFeatures) & FormatFlagBits) == FormatFlagBits)
        {
            vulk->defaultColorFormat = defaultFormats[j].format;
            break;
        }
    }

    ASSERT(vulk->defaultColorFormat != VK_FORMAT_UNDEFINED);

    PodVector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        vulk->queueFamilyIndices.graphicsFamily, vulk->queueFamilyIndices.presentFamily
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

    VulkanDeviceOptionals optionals = getDeviceOptionals(vulk->physicalDevice);
    // createdeviceinfo
    {
        static constexpr VkPhysicalDeviceFeatures deviceFeatures = {
            .samplerAnisotropy = VK_FALSE,
        };
        static constexpr  VkPhysicalDeviceVulkan13Features deviceFeatures13 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .dynamicRendering = VK_TRUE,
        };

        static constexpr  VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = VulkanApiVersion >= VK_API_VERSION_1_3 ? (void*)&deviceFeatures13 : nullptr,
            .features = deviceFeatures,
        };


        VkDeviceCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        createInfo.pNext = &physicalDeviceFeatures2;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();



        createInfo.pEnabledFeatures = nullptr;

        PodVector<const char*> deviceExts(deviceExtensions, deviceExtensions + ARRAYSIZES(deviceExtensions));
        if (optionals.canUseVulkanRenderdocExtensionMarker)
        {
            deviceExts.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        }
        createInfo.enabledExtensionCount = uint32_t(deviceExts.size());
        createInfo.ppEnabledExtensionNames = deviceExts.data();


        if (vulk->initParams.useValidationLayers)
        {
            createInfo.enabledLayerCount = ARRAYSIZES(validationLayers);
            createInfo.ppEnabledLayerNames = validationLayers;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        VK_CHECK(vkCreateDevice(vulk->physicalDevice, &createInfo, nullptr, &vulk->device));
        ASSERT(vulk->device);
        if (!vulk->device)
            return false;
    }

    vkGetDeviceQueue(vulk->device, vulk->queueFamilyIndices.graphicsFamily, 0, &vulk->graphicsQueue);
    vkGetDeviceQueue(vulk->device, vulk->queueFamilyIndices.presentFamily, 0, &vulk->presentQueue);
    vkGetDeviceQueue(vulk->device, vulk->queueFamilyIndices.computeFamily, 0, &vulk->computeQueue);
    ASSERT(vulk->graphicsQueue);
    ASSERT(vulk->presentQueue);
    ASSERT(vulk->computeQueue);


    if (!vulk->device || !vulk->graphicsQueue || !vulk->presentQueue || !vulk->computeQueue)
        return false;


    if(optionals.canUseVulkanRenderdocExtensionMarker)
        acquireDeviceDebugRenderdocFunctions(vulk->device);
    return true;
}







static VkDescriptorSetLayout createSetLayout(const PodVector<DescriptorSetLayout>& descriptors, VkShaderStageFlags stage)
{
    ASSERT(descriptors.size() > 0);
    PodVector<VkDescriptorSetLayoutBinding> setBindings(descriptors.size());
    for (uint32_t i = 0; i < uint32_t(descriptors.size()); ++i)
    {
        setBindings[i] = VkDescriptorSetLayoutBinding {
            .binding = uint32_t(descriptors[i].bindingIndex),
            .descriptorType = descriptors[i].descriptorType,
            .descriptorCount = 1,
            .stageFlags = stage, // VK_SHADER_STAGE_VERTEX_BIT;
        };
    }

    VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    //    createInfo.flags =  VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    createInfo.bindingCount = uint32_t(setBindings.size());
    createInfo.pBindings = setBindings.data();

    VkDescriptorSetLayout setLayout = 0;
    VK_CHECK(vkCreateDescriptorSetLayout(vulk->device, &createInfo, nullptr, &setLayout));

    ASSERT(setLayout);
    return setLayout;
}


static VkSemaphore createSemaphore()
{
    VkSemaphore semaphore = 0;
    VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VK_CHECK(vkCreateSemaphore(vulk->device, &semaphoreInfo, nullptr, &semaphore));
    return semaphore;
}

static VkCommandPool createCommandPool()
{
    uint32_t familyIndex = vulk->queueFamilyIndices.graphicsFamily;
    VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolCreateInfo.queueFamilyIndex = familyIndex;

    VkCommandPool commandPool = 0;
    VK_CHECK(vkCreateCommandPool(vulk->device, &poolCreateInfo, nullptr, &commandPool));

    return commandPool;
}


static VkRenderPass createRenderPass(const PodVector<RenderTarget>& colorTargets, const RenderTarget&depthFormat)
{
    bool hasDepthFormat = depthFormat.format != VK_FORMAT_UNDEFINED;
    PodVector< VkAttachmentDescription> attachments;
    PodVector< VkAttachmentReference > colorAttachments;
    for (const RenderTarget& target : colorTargets)
    {
        // To get indexes right
        colorAttachments.push_back({ VkAttachmentReference{ attachments.size(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL} });

        VkAttachmentDescription attachment = {};
        attachment.format = target.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = target.loadOp;
        attachment.storeOp = target.storeOp;
        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(attachment);
    }

    VkAttachmentReference depthAttachments = { colorAttachments.size(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    if(hasDepthFormat)
    {
        VkAttachmentDescription attachment = {};
        attachment.format = depthFormat.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = depthFormat.loadOp;
        attachment.storeOp = depthFormat.storeOp;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(attachment);
    }
    ASSERT(attachments.size());

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorAttachments.size();
    subpass.pColorAttachments = colorAttachments.size() > 0 ? colorAttachments.data() : nullptr;
    subpass.pDepthStencilAttachment = hasDepthFormat ? &depthAttachments : nullptr;

    VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    createInfo.attachmentCount = attachments.size();
    createInfo.pAttachments = attachments.size() > 0 ? attachments.data() : nullptr;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;

    VkRenderPass renderPass = 0;

    VK_CHECK(vkCreateRenderPass(vulk->device, &createInfo, nullptr, &renderPass));
    ASSERT(renderPass);
    return renderPass;
}



static VkQueryPool createQueryPool(uint32_t queryCount)
{
    VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };

    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = queryCount;

    VkQueryPool pool = 0;
    VK_CHECK(vkCreateQueryPool(vulk->device, &createInfo, nullptr, &pool));

    ASSERT(pool);
    return pool;
}


static VkFence createFence()
{
    VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence result = 0;
    VK_CHECK(vkCreateFence(vulk->device, &createInfo, nullptr, &result));
    ASSERT(result);
    return result;
}



static PodVector<DescriptorSetLayout> parseShaderLayouts(const PodVector<Shader> &shaders)
{
    PodVector< DescriptorSetLayout > result;

    for (uint32_t shaderIndex = 0; shaderIndex < shaders.size(); ++shaderIndex)
    {
        const Shader& shader = shaders[shaderIndex];
        for (uint32_t maskIndex = 0u; maskIndex < 64; ++maskIndex)
        {
            if ((shader.resourceMask & (uint64_t(1) << uint64_t(maskIndex))) == 0)
                continue;
            result.push_back(DescriptorSetLayout{ .descriptorType = shader.resourceTypes[maskIndex], .bindingIndex = maskIndex });
        }
    }
    return result;
}









bool initVulkan(VulkanApp &app, const VulkanInitializationParameters &initParameters)
{
    vulk = new VulkanGlobal();
    vulk->vulkanApp = &app;
    vulk->initParams = initParameters;
    ASSERT(app.window);
    if(!app.window)
    {
        printf("Empty window!\n");
        return false;
    }
    if(!createInstance())
    {
        printf("Failed to create vulkan instance!\n");
        return false;
    }

    vulk->debugCallBack = registerDebugCallback(vulk->instance);

    VK_CHECK(glfwCreateWindowSurface(vulk->instance, app.window, nullptr, &vulk->surface));
    ASSERT(vulk->surface);
    if(!vulk->surface)
    {
        printf("Failed to create vulkan surface!\n");
        return false;
    }


    if(!createPhysicalDevice(vulk->initParams.useIntegratedGpu ?
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU : VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
    {
        printf("Failed to create vulkan physical device!\n");
        return false;
    }
    ASSERT(vulk->physicalDevice);


    if (!createDeviceWithQueues())
    {
        printf("Failed to create vulkan device!\n");
        return false;
    }

    if (!initializeVMA())
    {
        printf("Failed to initialize VMA!\n");
        return false;
    }

    {
        VkPhysicalDeviceSubgroupProperties subgroupProperties;
        subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
        subgroupProperties.pNext = NULL;

        VkPhysicalDeviceProperties2 physicalDeviceProperties;
        physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        physicalDeviceProperties.pNext = &subgroupProperties;

        vkGetPhysicalDeviceProperties2(vulk->physicalDevice, &physicalDeviceProperties);

        printf("subgroup size: %u\n", subgroupProperties.subgroupSize);
        printf("subgroup operations: %u\n", subgroupProperties.supportedOperations);

    }

    bool scSuccess = createSwapchain(vulk->initParams.vsync);
    ASSERT(scSuccess);
    if(!scSuccess)
    {
        printf("Failed to create vulkan swapchain!\n");
        return false;
    }

    vulk->queryPool = createQueryPool(QUERY_COUNT);
    ASSERT(vulk->queryPool);
    if(!vulk->queryPool)
    {
        printf("Failed to create vulkan query pool!\n");
        return false;
    }

    vulk->acquireSemaphore = createSemaphore();
    ASSERT(vulk->acquireSemaphore);
    if(!vulk->acquireSemaphore)
    {
        printf("Failed to create vulkan acquire semapohore!\n");
        return false;
    }

    vulk->releaseSemaphore = createSemaphore();
    ASSERT(vulk->releaseSemaphore);
    if(!vulk->releaseSemaphore)
    {
        printf("Failed to create vulkan release semaphore!\n");
        return false;
    }

    vulk->fence = createFence();
    ASSERT(vulk->fence);
    if(!vulk->fence)
    {
        printf("Failed to create vulkan fence!\n");
        return false;
    }

    vulk->commandPool = createCommandPool();
    ASSERT(vulk->commandPool);
    if(!vulk->commandPool)
    {
        printf("Failed to create vulkan command pool!\n");
        return false;
    }

    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = vulk->commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VK_CHECK(vkAllocateCommandBuffers(vulk->device, &allocateInfo, &vulk->commandBuffer));
    if(!vulk->commandBuffer)
    {
        printf("Failed to create vulkan command buffer!\n");
        return false;
    }
    setObjectName((uint64_t)vulk->commandBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "Main command buffer");


    {
        vulk->scratchBuffer = createBuffer(64 * 1024 * 1024,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Scratch buffer");

        vulk->uniformBuffer = createBuffer(64u * 1024 * 1024,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Frame render uniform buffer");

        vulk->uniformBufferManager.init(vulk->uniformBuffer);
        vulk->renderFrameBufferHandle = vulk->uniformBufferManager.reserveHandle();
    }

    if (!loadShaders())
        return false;

    return true;
}




void deinitVulkan()
{

    if (vulk && vulk->device)
    {
        destroyBuffer(vulk->scratchBuffer);
        destroyBuffer(vulk->uniformBuffer);

        vkDestroyCommandPool(vulk->device, vulk->commandPool, nullptr);

        vkDestroyQueryPool(vulk->device, vulk->queryPool, nullptr);

        destroySwapchain(vulk->swapchain);

        deleteLoadedShaders();

        vkDestroyFence(vulk->device, vulk->fence, nullptr);
        vkDestroySemaphore(vulk->device, vulk->acquireSemaphore, nullptr);
        vkDestroySemaphore(vulk->device, vulk->releaseSemaphore, nullptr);
        deinitVMA();
        vkDestroyDevice(vulk->device, nullptr);
    }
    vkDestroySurfaceKHR(vulk->instance, vulk->surface, nullptr);

    if (vulk->initParams.useValidationLayers)
    {
        auto dest = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulk->instance, "vkDestroyDebugUtilsMessengerEXT");
        dest(vulk->instance, vulk->debugCallBack, nullptr);
    }
    vkDestroyInstance(vulk->instance, nullptr);
    delete vulk;
    vulk = nullptr;
}



bool resizeSwapchain()
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(vulk->vulkanApp->window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(vulk->vulkanApp->window, &width, &height);
    }

    uint32_t newWidth = uint32_t(width);
    uint32_t newHeight = uint32_t(height);

    if (vulk->swapchain.width == newWidth && vulk->swapchain.height == newHeight)
        return false;

    VK_CHECK(vkDeviceWaitIdle(vulk->device));

    SwapChain oldSwapchain = vulk->swapchain;
    createSwapchain(vulk->initParams.vsync);

    destroySwapchain(oldSwapchain);

    vulk->vulkanApp->resized();
    vulk->needToResize = false;
    return true;
}



bool createPipelineLayout(Pipeline& pipelineWithDescriptors, VkShaderStageFlags stage)
{
    VkDescriptorSetLayout setLayout = 0;
    VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

    if(pipelineWithDescriptors.descriptorSetLayouts.size() > 0)
        setLayout = createSetLayout(pipelineWithDescriptors.descriptorSetLayouts, stage);


    createInfo.setLayoutCount = setLayout ? 1 : 0;
    createInfo.pSetLayouts = setLayout ? &setLayout : nullptr;

    VkPipelineLayout layout = 0;
    VK_CHECK(vkCreatePipelineLayout(vulk->device, &createInfo, nullptr, &layout));
    ASSERT(layout);
    if (!layout)
        return false;

    pipelineWithDescriptors.pipelineLayout = layout;
    pipelineWithDescriptors.descriptorSetLayout = setLayout;
    return true;
}




















void beginSingleTimeCommands()
{
    VK_CHECK(vkResetCommandPool(vulk->device, vulk->commandPool, 0));

    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(vulk->commandBuffer, &beginInfo));

    return;
}

void endSingleTimeCommands()
{
    VK_CHECK(vkEndCommandBuffer(vulk->commandBuffer));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulk->commandBuffer;

    VkQueue queue = vulk->graphicsQueue;
    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(queue));
}

void beginRenderPass(const Pipeline& pipeline, const PodVector< VkClearValue >& clearValues)
{
    VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    passBeginInfo.renderPass = pipeline.renderPass;
    passBeginInfo.framebuffer = pipeline.framebuffer;
    passBeginInfo.renderArea.extent.width = pipeline.framebufferWidth;
    passBeginInfo.renderArea.extent.height = pipeline.framebufferHeight;
    passBeginInfo.clearValueCount = clearValues.size();
    passBeginInfo.pClearValues = clearValues.size() > 0 ? clearValues.data() : nullptr;

    vkCmdBeginRenderPass(vulk->commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkRect2D renderArea = { .extent = {.width = pipeline.framebufferWidth, .height = pipeline.framebufferHeight } };
    VkViewport viewPort = { 0.0f, float(pipeline.framebufferHeight), float(pipeline.framebufferWidth), -float(pipeline.framebufferHeight), 0.0f, 1.0f };
    VkRect2D scissors = { { 0, 0 }, { pipeline.framebufferWidth, pipeline.framebufferHeight } };
    vkCmdSetViewport(vulk->commandBuffer, 0, 1, &viewPort);
    vkCmdSetScissor(vulk->commandBuffer, 0, 1, &scissors);
}

void beginRendering(const PodVector<RenderImage> &renderColorImages, RenderImage depthImage)
{
    uint32_t width = 0u;
    uint32_t height = 0u;
    PodVector< VkRenderingAttachmentInfo> colorAttachments;
    for (const auto& renderImage : renderColorImages)
    {
        if (!renderImage.image)
            continue;

        if(width == 0u && height == 0u)
        {
            width = renderImage.image->width;
            height = renderImage.image->height;
        }
        else if (width != renderImage.image->width || height != renderImage.image->height)
        {
            ASSERT(!"Images are not same size\n");
            return;
        }
        const VkRenderingAttachmentInfo colorAttachmentInfo
        {
           .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
           .imageView = renderImage.image->imageView,
           .imageLayout = renderImage.image->layout,
           .loadOp = renderImage.loadOp,
           .storeOp = renderImage.storeOp,
           .clearValue = renderImage.clearValue
        };
        colorAttachments.push_back(colorAttachmentInfo);
    }
    VkRenderingAttachmentInfo depthAttachmentInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR  };
    if (depthImage.image)
    {
        if (width == 0u && height == 0u)
        {
            width = depthImage.image->width;
            height = depthImage.image->height;
        }
        else if (width != depthImage.image->width || height != depthImage.image->height)
        {
            ASSERT(!"Images are not same size\n");
            return;
        }
        depthAttachmentInfo.imageView = depthImage.image->imageView;
        depthAttachmentInfo.imageLayout = depthImage.image->layout;
        depthAttachmentInfo.loadOp = depthImage.loadOp;
        depthAttachmentInfo.storeOp = depthImage.storeOp;
        depthAttachmentInfo.clearValue = depthImage.clearValue;
    }

    VkRect2D renderArea = { .extent = {.width = width, .height = height } };
    VkViewport viewPort = { 0.0f, float(height), float(width), -float(height), 0.0f, 1.0f };
    VkRect2D scissors = { { 0, 0 }, { width, height } };

    const VkRenderingInfo renderInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = renderArea,
        .layerCount = 1,
        .colorAttachmentCount = colorAttachments.size(),
        .pColorAttachments = colorAttachments.data(),
        .pDepthAttachment = depthImage.image  ? &depthAttachmentInfo : nullptr,
    };

    vkCmdBeginRendering(vulk->commandBuffer, &renderInfo);
    vkCmdSetViewport(vulk->commandBuffer, 0, 1, &viewPort);
    vkCmdSetScissor(vulk->commandBuffer, 0, 1, &scissors);
}





bool startRender()
{
    VK_CHECK(vkWaitForFences(vulk->device, 1, &vulk->fence, VK_TRUE, UINT64_MAX));
    if (vulk->acquireSemaphore == VK_NULL_HANDLE)
        return false;
    VkResult res = ( vkAcquireNextImageKHR(vulk->device, vulk->swapchain.swapchain, UINT64_MAX,
        vulk->acquireSemaphore, VK_NULL_HANDLE, &vulk->imageIndex) );

    if (vulk->scratchBufferOffset > 32 * 1024 * 1024)
        vulk->scratchBufferOffset = 0u;
    else
        vulk->scratchBufferOffset = 32 * 1024 * 1024u;


    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if (resizeSwapchain())
        {
            VK_CHECK(vkDeviceWaitIdle(vulk->device));
        }
        return false;
    }
    else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
    {
        VK_CHECK(res);
        return false;
    }


    return true;
}

void present(Image & imageToPresent)
{
    // Copy final image to swap chain target
    {
        VkImageMemoryBarrier copyBeginBarriers[] =
        {
            imageBarrier(imageToPresent,
                VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),

            imageBarrier(vulk->swapchain.images[ vulk->imageIndex ],
                0, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        };

        vkCmdPipelineBarrier(vulk->commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZES(copyBeginBarriers), copyBeginBarriers);


        insertDebugRegion("Copy to swapchain", Vec4(1.0f, 1.0f, 0.0f, 1.0f));

        VkImageBlit imageBlitRegion = {};

        imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlitRegion.srcSubresource.layerCount = 1;
        imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlitRegion.dstSubresource.layerCount = 1;
        imageBlitRegion.srcOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
        imageBlitRegion.srcOffsets[ 1 ] = VkOffset3D{ ( int32_t ) vulk->swapchain.width, ( int32_t ) vulk->swapchain.height, 1 };
        imageBlitRegion.dstOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
        imageBlitRegion.dstOffsets[ 1 ] = VkOffset3D{ ( int32_t ) vulk->swapchain.width, ( int32_t ) vulk->swapchain.height, 1 };


        vkCmdBlitImage(vulk->commandBuffer,
            imageToPresent.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            vulk->swapchain.images[ vulk->imageIndex ], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VkFilter::VK_FILTER_NEAREST);
    }

    // Prepare image for presenting.
    {
        VkImageMemoryBarrier presentBarrier =
            imageBarrier(vulk->swapchain.images[ vulk->imageIndex ],
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vkCmdPipelineBarrier(vulk->commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &presentBarrier);
    }


    endDebugRegion();

    VK_CHECK(vkEndCommandBuffer(vulk->commandBuffer));

    // Submit
    {
        VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; //VK_PIPELINE_STAGE_TRANSFER_BIT;

        vkResetFences(vulk->device, 1, &vulk->fence);

        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &vulk->acquireSemaphore;
        submitInfo.pWaitDstStageMask = &submitStageMask;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vulk->commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &vulk->releaseSemaphore;
        VK_CHECK(vkQueueSubmit(vulk->graphicsQueue, 1, &submitInfo, vulk->fence));

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &vulk->releaseSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vulk->swapchain.swapchain;
        presentInfo.pImageIndices = &vulk->imageIndex;

        VkResult res = ( vkQueuePresentKHR(vulk->presentQueue, &presentInfo) );
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || vulk->needToResize)
        {
            resizeSwapchain();
            vulk->needToResize = false;
        }
        else
        {
            VK_CHECK(res);
        }
    }

    //VK_CHECK(vkDeviceWaitIdle(vulk->device));

}























bool createGraphicsPipeline(const Shader &vertShader, const Shader &fragShader,
    const PodVector<RenderTarget> &colorTargets, const DepthTest &depthTest, Pipeline &outPipeline,
    std::string_view pipelineName, bool useDynamic)
{
    destroyPipeline(outPipeline);

    bool hasFragShader = fragShader.module != nullptr;

    PodVector<Shader> shaders;

    if(hasFragShader)
        shaders = { vertShader, fragShader };
    else
        shaders = { vertShader };

    outPipeline.descriptorSetLayouts = parseShaderLayouts(shaders);
    if (!createPipelineLayout(outPipeline, VK_SHADER_STAGE_ALL_GRAPHICS))
    {
        printf("Failed to create graphics pipelinelayout!\n");
        return false;
    }

    PodVector<VkPipelineShaderStageCreateInfo> stageInfos;
    stageInfos.push_back({
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShader.module,
            .pName = "main",
        });
    if (hasFragShader)
    {
        stageInfos.push_back({
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShader.module,
            .pName = "main",
            });
    }
    

    VkPipelineVertexInputStateCreateInfo vertexInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    VkPipelineInputAssemblyStateCreateInfo assemblyInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
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
    depthInfo.depthTestEnable = depthTest.useDepthTest ? VK_TRUE : VK_FALSE;
    depthInfo.depthWriteEnable = depthTest.writeDepth ? VK_TRUE : VK_FALSE;
    depthInfo.depthCompareOp = depthTest.depthCompareOp;

    PodVector<VkPipelineColorBlendAttachmentState> colorAttachmentStates;
    colorAttachmentStates.resize(colorTargets.size());
    for (uint32_t i = 0; i < colorTargets.size(); ++i)
    {
        VkPipelineColorBlendAttachmentState colorAttachmentState{
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
        colorAttachmentStates[i] = colorAttachmentState;
    }
    VkPipelineColorBlendStateCreateInfo blendInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = colorAttachmentStates.size(),
        .pAttachments = colorAttachmentStates.size() > 0 ? colorAttachmentStates.data() : nullptr,
    };

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicInfo.pDynamicStates = dynamicStates;
    dynamicInfo.dynamicStateCount = ARRAYSIZES(dynamicStates);

    PodVector<VkFormat> colorFormats;
    for (const auto& target : colorTargets)
    {
        colorFormats.push_back(target.format);
    }

    const VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = colorFormats.size(),
        .pColorAttachmentFormats = colorFormats.data(),
        .depthAttachmentFormat = depthTest.depthTarget.format,
    };
    VkRenderPass renderPass = nullptr;
    if (!useDynamic)
    {
        renderPass = createRenderPass(colorTargets, depthTest.depthTarget);
        ASSERT(renderPass);
        if (!renderPass)
            return false;
    }
    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    createInfo.stageCount = stageInfos.size();
    createInfo.pStages = stageInfos.size() > 0 ? stageInfos.data() : nullptr;
    createInfo.pVertexInputState = &vertexInfo;
    createInfo.pInputAssemblyState = &assemblyInfo;
    createInfo.pViewportState = &viewportInfo;
    createInfo.pRasterizationState = &rasterInfo;
    createInfo.pMultisampleState = &multiSampleInfo;
    createInfo.pDepthStencilState = &depthInfo;
    createInfo.pColorBlendState = &blendInfo;
    createInfo.pDynamicState = &dynamicInfo;
    createInfo.renderPass = renderPass; // VK_NULL_HANDLE;
    createInfo.layout = outPipeline.pipelineLayout;
    createInfo.basePipelineHandle = VK_NULL_HANDLE;
    if(useDynamic)
        createInfo.pNext = &pipelineRenderingCreateInfo;

    VkPipeline pipeline = 0;
    VK_CHECK(vkCreateGraphicsPipelines(vulk->device, nullptr, 1, &createInfo, nullptr, &pipeline));
    ASSERT(pipeline);

    outPipeline.pipeline = pipeline;
    outPipeline.renderPass = renderPass;

    if (!createDescriptor(outPipeline))
    {
        printf("Failed to create graphics pipeline descriptor\n");
        return false;
    }
    setObjectName((uint64_t)pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, pipelineName);

    return pipeline != nullptr;
}


bool createComputePipeline(const Shader &csShader, Pipeline &outPipeline, std::string_view pipelineName)
{
    destroyPipeline(outPipeline);

    PodVector<Shader> shaders = { csShader };
    outPipeline.descriptorSetLayouts = parseShaderLayouts(shaders);
    if (!createPipelineLayout(outPipeline, VK_SHADER_STAGE_COMPUTE_BIT))
    {
        printf("Failed to create graphics pipelinelayout!\n");
        return false;
    }

    VkPipelineShaderStageCreateInfo stageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = csShader.module;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    createInfo.stage = stageInfo;
    createInfo.layout = outPipeline.pipelineLayout;

    VkPipeline pipeline = 0;
    VK_CHECK(vkCreateComputePipelines(vulk->device, nullptr, 1, &createInfo, nullptr, &pipeline));
    ASSERT(pipeline);

    outPipeline.pipeline = pipeline;
    
    if (!createDescriptor(outPipeline))
    {
        printf("Failed to create compute pipeline descriptor\n");
        return false;
    }
    setObjectName((uint64_t)pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, pipelineName);
    return pipeline != nullptr;
}


void destroyPipeline(Pipeline &pipeline)
{
    destroyDescriptor(pipeline.descriptor);
    destroyFramebuffer(pipeline.framebuffer);

    if (pipeline.renderPass)
        vkDestroyRenderPass(vulk->device, pipeline.renderPass, nullptr);
    if(pipeline.pipeline)
        vkDestroyPipeline(vulk->device, pipeline.pipeline, nullptr);
    if(pipeline.pipelineLayout)
        vkDestroyPipelineLayout(vulk->device, pipeline.pipelineLayout, nullptr );

    if(pipeline.descriptorSetLayout)
        vkDestroyDescriptorSetLayout(vulk->device, pipeline.descriptorSetLayout, nullptr);

    pipeline.pipeline = nullptr;
    pipeline.pipelineLayout = nullptr;
    pipeline.descriptorSetLayout = nullptr;
    pipeline.renderPass = nullptr;
    pipeline.framebuffer = nullptr;
}

void destroyDescriptor(Descriptor& descriptor)
{
    if (descriptor.pool)
    {
        vkDestroyDescriptorPool(vulk->device, descriptor.pool, nullptr);
        descriptor.pool = 0;
        descriptor.descriptorSet = 0;
    }
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
        pfnDebugMarkerSetObjectName(vulk->device, &nameInfo);
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
        pfnDebugMarkerSetObjectTag(vulk->device, &tagInfo);
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
//        pfnCmdDebugMarkerBegin(vulk->commandBuffer, &markerInfo);
//    }
}

// Insert a new debug marker into the command buffer
void insertDebugRegion(std::string_view markerName, Vec4 color)
{
    // Check for valid function pointer (may not be present if not running in a debugging application)
    if (pfnCmdDebugMarkerInsert)
    {
        VkDebugMarkerMarkerInfoEXT markerInfo = {};
        markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
        memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
        markerInfo.pMarkerName = markerName.data();
        pfnCmdDebugMarkerInsert(vulk->commandBuffer, &markerInfo);
    }
}

// End the current debug marker region
void endDebugRegion()
{
    // Check for valid function (may not be present if not runnin in a debugging application)
    if (pfnCmdDebugMarkerEnd)
    {
        pfnCmdDebugMarkerEnd(vulk->commandBuffer);
    }
}


void bindPipelineWithDecriptors(VkPipelineBindPoint bindPoint, const Pipeline &pipelineWithDescriptor)
{
    vkCmdBindPipeline(vulk->commandBuffer, bindPoint, pipelineWithDescriptor.pipeline);
    vkCmdBindDescriptorSets(vulk->commandBuffer, bindPoint, pipelineWithDescriptor.pipelineLayout,
        0, 1, &pipelineWithDescriptor.descriptor.descriptorSet, 0, NULL);
}

