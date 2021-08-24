#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>


struct BufferHandle
{
public:
	bool isValid() { return (handleId != 0u); }
	explicit operator bool() { return (handleId != 0u); }

	uint32_t handleId = 0u;
	uint32_t isFreed = 1u;
};

struct AllocationHandle
{
public:
	bool isValid() { return (handleId != 0u); }
	explicit operator bool() { return (handleId != 0u); }

	uint32_t handleId = 0u;
};

struct MyBuffer
{
	VkDeviceMemory memory;
	VkDeviceSize size = 0;
	VkDeviceSize offset = 0;
	AllocationHandle handle;
};

struct Allocation
{
	VkDeviceMemory memory;
	uint32_t type;
	uint32_t id;
	VkDeviceSize size;
	VkDeviceSize allocatedAmount;

	std::vector<BufferHandle> handles;
	std::vector<BufferHandle> freeHandles;
};



class VulkanMemoryManager
{
public:
	VulkanMemoryManager(VkDevice device);
	~VulkanMemoryManager();
	bool init();

	VkBuffer getBuffer(BufferHandle handle);
	void deleteBuffer(BufferHandle handle);



private:
	VkDevice device;
	VkPhysicalDevice physicalDevice;

	std::vector<Allocation> allocations;
};