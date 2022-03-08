#include "vulkanswapchain.h"

#include <algorithm>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "vulkandevice.h"
#include "core/mytypes.h"



static VkSwapchainKHR createSwapchain(GLFWwindow *window, DeviceWithQueues &deviceWithQueues, VkSwapchainKHR oldSwapchain)
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(deviceWithQueues.physicalDevice, deviceWithQueues.surface);
    ASSERT(swapChainSupport.formats.size() > 0);
    VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
    bool found = false;
    for (const auto& availableFormat : swapChainSupport.formats)
    {
        if (availableFormat.format == deviceWithQueues.colorFormat && availableFormat.colorSpace == deviceWithQueues.colorSpace)
        {
            surfaceFormat = availableFormat;
            found = true;
            break;
        }
    }
    if(!found && swapChainSupport.formats.size() == 1 && swapChainSupport.formats[0].format == VK_FORMAT_UNDEFINED)
    {
        surfaceFormat.colorSpace = deviceWithQueues.colorSpace;
        surfaceFormat.format = deviceWithQueues.colorFormat;
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
    createInfo.surface = deviceWithQueues.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.oldSwapchain = oldSwapchain;

    QueueFamilyIndices indices = findQueueFamilies(deviceWithQueues.physicalDevice, deviceWithQueues.surface);
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

    VkSwapchainKHR swapChain = 0;
//    PreCallValidateCreateSwapchainKHR()
    [[maybe_unused]] VkResult res = vkCreateSwapchainKHR(deviceWithQueues.device, &createInfo, nullptr, &swapChain);
    if (res != VK_SUCCESS)
    {
        LOG("Failed to initialize swapchain\n");
    }
    VK_CHECK(res);

    return swapChain;
}


SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
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




void destroySwapchain(SwapChain &swapchain, VkDevice device)
{
    swapchain.images.clear();
    swapchain.swapchainCount = 0u;
    swapchain.width = swapchain.height = 0u;

    if(swapchain.swapchain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);
    swapchain.swapchain = VK_NULL_HANDLE;

}

bool createSwapchain(SwapChain &swapChain, GLFWwindow *window, DeviceWithQueues &deviceWithQueues)
{
    VkDevice device = deviceWithQueues.device;
    swapChain.swapchain = createSwapchain(window, deviceWithQueues, swapChain.swapchain);

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

bool resizeSwapchain(SwapChain &swapChain, GLFWwindow *window, DeviceWithQueues &deviceWithQueues)
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

    VK_CHECK(vkDeviceWaitIdle(deviceWithQueues.device));

    SwapChain oldSwapchain = swapChain;
    createSwapchain(swapChain, window, deviceWithQueues);

    destroySwapchain(oldSwapchain, deviceWithQueues.device);
    return true;
}

