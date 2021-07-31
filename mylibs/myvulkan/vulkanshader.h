#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>

#include <core/mytypes.h>


struct Buffer;

// Probably bit clumsy for vertices
struct VertexInput
{
	std::vector<VkFormat> formats;
};

struct Pipeline
{
	std::vector<VkPipeline> pipelines;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkShaderStageFlags pushConstantStage;
};

struct DescriptorSet
{
	VkShaderStageFlags stage = 0;
	VkDescriptorType descriptorType;
	u32 bindingIndex = ~0u;
	bool writeDescriptor = true;
	const Buffer *buffer = nullptr;
	VkImage image = 0;
	VkImageView imageView = 0;
	VkSampler sampler = 0;
	VkImageLayout layout;
};

struct Descriptor
{
	VkDescriptorSet descriptorSet = 0;
	VkDescriptorPool pool = 0;
};

struct DescriptorInfo
{
	enum class DescriptorType
	{
		IMAGE,
		BUFFER,

		NOT_VALID
	};

	DescriptorInfo(VkSampler sampler, VkImageView imageView, VkImageLayout layout)
	{

		type = DescriptorType::IMAGE;
	}

	DescriptorInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
	{
		bufferInfo.buffer = buffer;
		bufferInfo.offset = offset;
		bufferInfo.range = range;
		type = DescriptorType::BUFFER;
	}

	VkDescriptorImageInfo imageInfo;
	VkDescriptorBufferInfo bufferInfo;

	DescriptorType type = DescriptorType::NOT_VALID; 
};



VkShaderModule loadShader(VkDevice device, const char *path);

Pipeline createGraphicsPipeline(VkDevice device, VkRenderPass renderPass, VkPipelineCache cache, VkShaderModule vs, VkShaderModule fs, const VertexInput &vertexInput, 
	const std::vector<DescriptorSet> &descriptors, bool depthTest, size_t pushConstantSize = 0, VkShaderStageFlagBits pushConstantStage  = VK_SHADER_STAGE_ALL);
Pipeline createComputePipeline(VkDevice device, VkPipelineCache cache, VkShaderModule cs, 
	const std::vector<DescriptorSet> &descriptors, size_t pushConstantSize = 0, VkShaderStageFlagBits pushConstantStage  = VK_SHADER_STAGE_ALL);

Descriptor createDescriptor(VkDevice device, const std::vector<DescriptorSet> &descriptors, VkDescriptorSetLayout descriptorSetLayout);
void destroyDescriptor(VkDevice device, Descriptor &descriptor);

void destroyPipeline(VkDevice device, Pipeline &pipeline);

