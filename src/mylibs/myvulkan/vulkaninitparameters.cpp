#include "vulkaninitparameters.h"

static VulkanInitializationParameters vulkanInitParams;

const VulkanInitializationParameters &VulkanInitializationParameters::get()
{
    return vulkanInitParams;
}

VulkanInitializationParameters &VulkanInitializationParameters::getRef()
{
    return vulkanInitParams;
}

