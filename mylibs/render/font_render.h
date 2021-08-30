#pragma once

#include "math/vector3.h"
#include "myvulkan/vulkandevice.h"
#include "myvulkan/vulkanhelperfuncs.h"
#include "myvulkan/vulkanresource.h"

static constexpr uint32_t MAX_LETTERS = 10000 * 4;

class FontRenderSystem
{
public:
	FontRenderSystem() {}
	~FontRenderSystem() {}
	bool init(std::string_view fontFilename, DeviceWithQueues &deviceWithQueues, VkRenderPass renderPass,
		VkPipelineCache pipelineCache,  
		Buffer& scratchBuffer, const Buffer& renderFrameBuffer);
	void deInit(VkDevice device);
	//void update(VkDevice device, VkCommandBuffer commandBuffer,
	//	VkRenderPass renderPass, Vector2 renderAreaSize, Buffer& scratchBuffer);
	// return offset to scratch buffer
	uint32_t update(VkDevice device, VkCommandBuffer commandBuffer,
		VkRenderPass renderPass, Vector2 renderAreaSize, Buffer& scratchBuffer,
		uint32_t offset);
	void render(VkCommandBuffer commandBuffer);

	void addText(std::string_view text, Vector2 pos, 
		Vector2 charSize = Vector2(8.0f, 12.0f), const Vector4 &color = Vector4(1.0f, 1.0f, 1.0f, 1.0f));

private:
	struct GPUVertexData
	{
		float posX;
		float posY;
		uint16_t pixelSizeX;
		uint16_t pixelSizeY;
		uint32_t color;

		float uvX;
		float uvY;

		float padding[2];
	};

	std::vector<GPUVertexData> vertData;

	VkShaderModule vertexShader = nullptr;
	VkShaderModule fragShader = nullptr;

	Buffer letterDataBuffer;
	Buffer letterIndexBuffer;

	Image textImage;
	PipelineWithDescriptors pipelinesWithDescriptor;

	VkSampler textureSampler = 0;
};

