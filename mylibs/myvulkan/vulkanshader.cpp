#include "vulkanshader.h"

#include <assert.h>
#include <vector>

#include "core/general.h"
#include "core/mytypes.h"

#include "vulkanresource.h"

static VkDescriptorSetLayout createSetLayout(VkDevice device, const std::vector<DescriptorSet> &descriptors)
{
	ASSERT(descriptors.size() > 0);
	std::vector<VkDescriptorSetLayoutBinding> setBindings(descriptors.size());
	for(u32 i = 0; i < u32(descriptors.size()); ++i)
	{
		setBindings[i].binding = descriptors[i].bindingIndex;
		setBindings[i].descriptorType = descriptors[i].descriptorType; // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		setBindings[i].descriptorCount = 1;
		setBindings[i].stageFlags = descriptors[i].stage; // VK_SHADER_STAGE_VERTEX_BIT;
	}

	VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
//	createInfo.flags =  VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
	createInfo.bindingCount = u32(setBindings.size());
	createInfo.pBindings = setBindings.data();

	VkDescriptorSetLayout setLayout = 0;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &setLayout));
	ASSERT(setLayout);
	return setLayout;
}

static Pipeline createPipelineLayout(VkDevice device, const std::vector<DescriptorSet> &descriptors, size_t pushConstantSize, VkShaderStageFlagBits pushConstantStage)
{
	VkDescriptorSetLayout setLayout = 0;
	VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	if(descriptors.size() > 0)
		setLayout = createSetLayout(device, descriptors);


	createInfo.setLayoutCount = setLayout ? 1 : 0;
	createInfo.pSetLayouts = setLayout ? &setLayout : nullptr;
	
	VkPushConstantRange pushConstantRange = {};
	if(pushConstantSize > 0u)
	{
		pushConstantRange.stageFlags = pushConstantStage;
		pushConstantRange.size = (u32)pushConstantSize;

		createInfo.pushConstantRangeCount = 1;
		createInfo.pPushConstantRanges = &pushConstantRange;
	}

	VkPipelineLayout layout = 0;
	VK_CHECK(vkCreatePipelineLayout(device, &createInfo, nullptr, &layout));
	ASSERT(layout);

	//vkDestroyDescriptorSetLayout(device, setLayout, nullptr);

	Pipeline result;
	result.pipelineLayout = layout;
	result.descriptorSetLayout = setLayout; 
	return result;
}

Descriptor createDescriptor(VkDevice device, const std::vector<DescriptorSet> &descriptors, VkDescriptorSetLayout descriptorSetLayout)
{
	Descriptor result;
	if(descriptors.size() == 0)
		return result;

	std::vector<VkDescriptorPoolSize> poolSizes(descriptors.size());
	u32 writeDescriptorCount = (u32)descriptors.size();

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

	if(writeDescriptorCount > 0u)
	{
		std::vector<VkWriteDescriptorSet> writeDescriptorSets(writeDescriptorCount);
		std::vector<VkDescriptorBufferInfo> bufferInfos(writeDescriptorCount);

		std::vector<VkDescriptorImageInfo> imageInfos(writeDescriptorCount);

		u32 writeIndex = 0u;
		u32 bufferCount = 0u;
		u32 imageCount = 0u;
		for(u32 i = 0; i < descriptors.size(); ++i)
		{
			if(descriptors[ i ].buffer)
			{
				ASSERT(descriptors[ i ].buffer);
				ASSERT(descriptors[ i ].buffer->size > 0);
				bufferInfos[bufferCount] = {};
				bufferInfos[bufferCount].buffer = descriptors[i].buffer->buffer;
				bufferInfos[bufferCount].offset = descriptors[i].offset;
				if(descriptors[i].size == 0)
					bufferInfos[bufferCount].range = descriptors[i].buffer->size;
				else
					bufferInfos[bufferCount].range = descriptors[i].size;
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
			else
			{
				ASSERT(descriptors[ i ].sampler);
				ASSERT(descriptors[ i ].imageView);
				ASSERT(descriptors[ i ].layout);

				imageInfos[imageCount] = {};
				imageInfos[imageCount].sampler = descriptors[i].sampler;
				imageInfos[imageCount].imageView = descriptors[i].imageView;
				imageInfos[imageCount].imageLayout = descriptors[i].layout;

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
		}

		if(writeDescriptorSets.size() > 0)
		{
			vkUpdateDescriptorSets(device, u32(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	return result;
}


void destroyDescriptor(VkDevice device, Descriptor &descriptor)
{
	vkDestroyDescriptorPool(device, descriptor.pool, nullptr);
	descriptor.pool = 0;
	descriptor.descriptorSet = 0;
}


Pipeline createGraphicsPipeline(VkDevice device, VkRenderPass renderPass, VkPipelineCache cache, VkShaderModule vs, VkShaderModule fs, const VertexInput &vertexInput, 
	const std::vector<DescriptorSet> &descriptors, bool depthTest, size_t pushConstantSize, VkShaderStageFlagBits pushConstantStage)
{
	Pipeline result = createPipelineLayout(device, descriptors, pushConstantSize, pushConstantStage);
	ASSERT(result.pipelineLayout);

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
	std::vector<VkVertexInputAttributeDescription> attrs;
	VkVertexInputBindingDescription stream;
	if(vertexInput.formats.size() > 0)
	{
		u32 streamSize = 0u;
		attrs.resize(vertexInput.formats.size());
		for(u32 i = 0; i < u32(vertexInput.formats.size()); ++i)
		{
			attrs[i].location = i;
			attrs[i].format = vertexInput.formats[i];
			attrs[i].offset = streamSize;


			u32 fSize = 0u;
			switch(vertexInput.formats[i])
			{
				// 1byte
				case VK_FORMAT_R8_UNORM:
				case VK_FORMAT_R8_SNORM:
				case VK_FORMAT_R8_USCALED:
				case VK_FORMAT_R8_SSCALED:
				case VK_FORMAT_R8_UINT:
				case VK_FORMAT_R8_SINT:
				case VK_FORMAT_R8_SRGB:
					fSize = 1 * 1u;
					break;

				// 2 bytes
				case VK_FORMAT_R8G8_UNORM:
				case VK_FORMAT_R8G8_SNORM:
				case VK_FORMAT_R8G8_USCALED:
				case VK_FORMAT_R8G8_SSCALED:
				case VK_FORMAT_R8G8_UINT:
				case VK_FORMAT_R8G8_SINT:
				case VK_FORMAT_R8G8_SRGB:
				case VK_FORMAT_R16_UNORM:
				case VK_FORMAT_R16_SNORM:
				case VK_FORMAT_R16_USCALED:
				case VK_FORMAT_R16_SSCALED:
				case VK_FORMAT_R16_UINT:
				case VK_FORMAT_R16_SINT:
				case VK_FORMAT_R16_SFLOAT:
					fSize = 2 * 1u;
					break;

				// 3 bytes
				case VK_FORMAT_R8G8B8_UNORM:
				case VK_FORMAT_R8G8B8_SNORM:
				case VK_FORMAT_R8G8B8_USCALED:
				case VK_FORMAT_R8G8B8_SSCALED:
				case VK_FORMAT_R8G8B8_UINT:
				case VK_FORMAT_R8G8B8_SINT:
				case VK_FORMAT_R8G8B8_SRGB:
				case VK_FORMAT_B8G8R8_UNORM:
				case VK_FORMAT_B8G8R8_SNORM:
				case VK_FORMAT_B8G8R8_USCALED:
				case VK_FORMAT_B8G8R8_SSCALED:
				case VK_FORMAT_B8G8R8_UINT:
				case VK_FORMAT_B8G8R8_SINT:
				case VK_FORMAT_B8G8R8_SRGB:
					fSize = 3 * 1u;
					break;

				// 4 bytes
				case VK_FORMAT_R8G8B8A8_UNORM:
				case VK_FORMAT_R8G8B8A8_SNORM:
				case VK_FORMAT_R8G8B8A8_USCALED:
				case VK_FORMAT_R8G8B8A8_SSCALED:
				case VK_FORMAT_R8G8B8A8_UINT:
				case VK_FORMAT_R8G8B8A8_SINT:
				case VK_FORMAT_R8G8B8A8_SRGB:
				case VK_FORMAT_B8G8R8A8_UNORM:
				case VK_FORMAT_B8G8R8A8_SNORM:
				case VK_FORMAT_B8G8R8A8_USCALED:
				case VK_FORMAT_B8G8R8A8_SSCALED:
				case VK_FORMAT_B8G8R8A8_UINT:
				case VK_FORMAT_B8G8R8A8_SINT:
				case VK_FORMAT_B8G8R8A8_SRGB:
				case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
				case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
				case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
				case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
				case VK_FORMAT_A8B8G8R8_UINT_PACK32:
				case VK_FORMAT_A8B8G8R8_SINT_PACK32:
				case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
				case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
				case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
				case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
				case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
				case VK_FORMAT_A2R10G10B10_UINT_PACK32:
				case VK_FORMAT_A2R10G10B10_SINT_PACK32:
				case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
				case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
				case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
				case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
				case VK_FORMAT_A2B10G10R10_UINT_PACK32:
				case VK_FORMAT_A2B10G10R10_SINT_PACK32:
				case VK_FORMAT_R16G16_UNORM:
				case VK_FORMAT_R16G16_SNORM:
				case VK_FORMAT_R16G16_USCALED:
				case VK_FORMAT_R16G16_SSCALED:
				case VK_FORMAT_R16G16_UINT:
				case VK_FORMAT_R16G16_SINT:
				case VK_FORMAT_R16G16_SFLOAT:
				case VK_FORMAT_R32_UINT:
				case VK_FORMAT_R32_SINT:
				case VK_FORMAT_R32_SFLOAT:
					fSize = 2 * 2u;
					break;


				// 6 bytes
				case VK_FORMAT_R16G16B16_UNORM:
				case VK_FORMAT_R16G16B16_SNORM:
				case VK_FORMAT_R16G16B16_USCALED:
				case VK_FORMAT_R16G16B16_SSCALED:
				case VK_FORMAT_R16G16B16_UINT:
				case VK_FORMAT_R16G16B16_SINT:
				case VK_FORMAT_R16G16B16_SFLOAT:
					fSize = 3 * 2u;
					break;


				// 8 bytes
				case VK_FORMAT_R16G16B16A16_UNORM:
				case VK_FORMAT_R16G16B16A16_SNORM:
				case VK_FORMAT_R16G16B16A16_USCALED:
				case VK_FORMAT_R16G16B16A16_SSCALED:
				case VK_FORMAT_R16G16B16A16_UINT:
				case VK_FORMAT_R16G16B16A16_SINT:
				case VK_FORMAT_R16G16B16A16_SFLOAT:
				case VK_FORMAT_R32G32_UINT:
				case VK_FORMAT_R32G32_SINT:
				case VK_FORMAT_R32G32_SFLOAT:
					fSize = 2u * 4u;
					break;

				// 12 bytes
				case VK_FORMAT_R32G32B32_UINT:
				case VK_FORMAT_R32G32B32_SINT:
				case VK_FORMAT_R32G32B32_SFLOAT:
					fSize = 3u * 4u;
					break;

				// 16 bytes
				case VK_FORMAT_R32G32B32A32_UINT:
				case VK_FORMAT_R32G32B32A32_SINT:
				case VK_FORMAT_R32G32B32A32_SFLOAT:
					fSize = 4u * 4u;
					break;

				default:
					printf("%d not found\n", vertexInput.formats[i]);
					break;
			}
			ASSERT(fSize > 0u && "Undefined format");

			streamSize += fSize;
		}
		if(streamSize >= 16)
			ASSERT((streamSize % 16 == 0) && "Not divisable by 16 bytes");
		stream = {0, streamSize, VK_VERTEX_INPUT_RATE_VERTEX };

		vertexInfo.vertexBindingDescriptionCount = 1;
		vertexInfo.pVertexBindingDescriptions = &stream;
		vertexInfo.vertexAttributeDescriptionCount = u32(attrs.size());
		vertexInfo.pVertexAttributeDescriptions = attrs.data();
	}

	VkPipelineInputAssemblyStateCreateInfo assemblyInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportInfo.scissorCount = 1;
	viewportInfo.viewportCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterInfo.lineWidth = 1.0f;
	rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	VkPipelineMultisampleStateCreateInfo multiSampleInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multiSampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	if (depthTest)
	{
		depthInfo.depthTestEnable = VK_TRUE;
		depthInfo.depthWriteEnable = VK_TRUE;
	}
	depthInfo.depthCompareOp = VK_COMPARE_OP_GREATER;

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
	createInfo.layout = result.pipelineLayout;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline pipeline = 0;
 	VK_CHECK(vkCreateGraphicsPipelines(device, cache, 1, &createInfo, nullptr, &pipeline));
	ASSERT(pipeline);

	result.pipelines.push_back(pipeline);
	result.pushConstantStage = pushConstantStage;
	return result;
}


Pipeline createComputePipeline(VkDevice device, VkPipelineCache cache, VkShaderModule cs, 
	const std::vector<DescriptorSet> &descriptors, size_t pushConstantSize, VkShaderStageFlagBits pushConstantStage)
{
	Pipeline result = createPipelineLayout(device, descriptors, pushConstantSize, pushConstantStage);
	ASSERT(result.pipelineLayout);

	VkPipelineShaderStageCreateInfo stageInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageInfo.module = cs;
	stageInfo.pName = "main";

	VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	createInfo.stage = stageInfo;
	createInfo.layout = result.pipelineLayout;

	VkPipeline pipeline = 0;
	VK_CHECK(vkCreateComputePipelines(device, cache, 1, &createInfo, nullptr, &pipeline ));
	ASSERT(pipeline);

	result.pipelines.push_back(pipeline);
	result.pushConstantStage = pushConstantStage;
	return result;
}


void destroyPipeline(VkDevice device, Pipeline &pipeline)
{
	for(u32 i = 0; i < u32(pipeline.pipelines.size()); ++i)
		vkDestroyPipeline(device, pipeline.pipelines[i], nullptr);
	if(pipeline.pipelineLayout)
		vkDestroyPipelineLayout(device, pipeline.pipelineLayout, nullptr );

	if(pipeline.descriptorSetLayout)
		vkDestroyDescriptorSetLayout(device, pipeline.descriptorSetLayout, nullptr);

	pipeline = Pipeline {};
}


VkShaderModule loadShader(VkDevice device, std::string_view filename)
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
