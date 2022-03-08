#include "vulkanmemorymanager.h"

VulkanMemoryManager::VulkanMemoryManager(VkDevice device)
{
    this->device = device;

}

VulkanMemoryManager::~VulkanMemoryManager()
{

}


bool VulkanMemoryManager::init()
{
    return true;
}