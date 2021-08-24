#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/mytypes.h"
#include "myvulkan/vulkanshader.h"


struct PipelineWithDescriptors
{
	Pipeline pipeline; // Maybe multiple?
	
	Descriptor descriptor; // maybe needs more than one, possibly separated from each other?
	std::vector<DescriptorSet> descriptorSet; // maybe needs more than one set?
};

//?
static void bindPipelineWithDecriptors(VkCommandBuffer cmdBuffer, VkPipelineBindPoint bindPoint, const PipelineWithDescriptors &pipelineWithDescriptor)
{
	vkCmdBindPipeline(cmdBuffer, bindPoint, pipelineWithDescriptor.pipeline.pipelines[0]);
	vkCmdBindDescriptorSets(cmdBuffer, bindPoint, pipelineWithDescriptor.pipeline.pipelineLayout,
		0, 1, &pipelineWithDescriptor.descriptor.descriptorSet, 0, NULL);
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




