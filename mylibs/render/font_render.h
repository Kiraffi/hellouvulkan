#include "math/vector3.h"
#include "myvulkan/vulkandevice.h"
#include "myvulkan/vulkanhelperfuncs.h"
#include "myvulkan/vulkanresource.h"

static constexpr uint32_t MAX_LETTERS = 10000 * 4;

class FontRenderSystem
{
public:
	FontRenderSystem() {}
	~FontRenderSystem() {};
	bool init(const std::string& fontFilename, VkDevice device, VkPhysicalDevice physicalDevice,
		VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkRenderPass renderPass, 
		VkPipelineCache pipelineCache, DeviceWithQueues& deviceWithQueues, 
		Buffer& scratchBuffer, const Buffer& renderFrameBuffer);
	void deInit(VkDevice device);
	//void update(VkDevice device, VkCommandBuffer commandBuffer,
	//	VkRenderPass renderPass, Vector2 renderAreaSize, Buffer& scratchBuffer);
	// return offset to scratch buffer
	uint32_t update(VkDevice device, VkCommandBuffer commandBuffer,
		VkRenderPass renderPass, Vector2 renderAreaSize, Buffer& scratchBuffer,
		uint32_t offset);
	void render(VkCommandBuffer commandBuffer);

	void addText(const std::string& text, Vector2 pos, Vec2 charSize);

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

	VkShaderModule vertexShader;
	VkShaderModule fragShader;

	Buffer letterDataBuffer;
	Buffer letterIndexBuffer;

	Image textImage;
	PipelineWithDescriptors pipelinesWithDescriptor;

	VkSampler textureSampler = 0;
};

