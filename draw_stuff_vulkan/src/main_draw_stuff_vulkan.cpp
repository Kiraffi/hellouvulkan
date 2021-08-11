#include <cstdio>
#include <cstdlib>
#include <cstdint>
// memcpy...
#include <string.h>

#include "core/general.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

#include "core/vulkan_app.h"

#include "core/timer.h"
#include "core/mytypes.h"

#include "myvulkan/vulkandevice.h"
#include "myvulkan/vulkanhelperfuncs.h"
#include "myvulkan/vulkanresource.h"
#include "myvulkan/vulkanshader.h"
#include "myvulkan/vulkanswapchain.h"

#include "math/general_math.h"
#include "math/matrix.h"
#include "math/plane.h"
#include "math/quaternion.h"
#include "math/vector3.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>


static constexpr int SCREEN_WIDTH = 640;
static constexpr int SCREEN_HEIGHT = 540;

struct GPUVertexData
{
	float posX;
	float posY;
	uint16_t pixelSizeX;
	uint16_t pixelSizeY;
	uint32_t color;
};



bool saveFontData(const std::string& filename, const std::vector<char>& data)
{
	//
	if (std::filesystem::exists(filename))
	{
		std::filesystem::path p(filename);


		std::ofstream f(p, std::ios::out | std::ios::binary);


		f.write(data.data(), data.size());

		printf("filesize: %u\n", uint32_t(data.size()));
		return true;
	}
	return false;
}


struct JSONBlock
{
	JSONBlock(int start) { startIndex = start; }
	JSONBlock(int start, int end) : startIndex(start), endIndex(end) {}

	bool isBracket() { return vvalid && !varray && vbracket; }
	bool isArray() { return vvalid && varray && !vbracket; }
	bool isValid() { return vvalid && !(varray && vbracket); }

	int startIndex = 0;
	int endIndex = 0;
	int currentIndex = 0;
	bool varray = false;
	bool vbracket = false;
	bool vvalid = false;
};


enum TIME_POINTS
{
	START_POINT,
	DRAW_FINISHED,

	NUM_TIME_POINTS
};


// Probably not good in long run?
enum PipelineWithDescriptorsIndexes
{
	PIPELINE_GRAPHICS_PIPELINE,
	NUM_PIPELINE
};

enum ShaderModuleIndexes
{
	SHADER_MODULE_RENDER_QUAD_VERT,
	SHADER_MODULE_RENDER_QUAD_FRAG,

	NUM_SHADER_MODULES
};

enum BufferIndexes
{
	QUAD_BUFFER,

	INDEX_DATA_BUFFER,

	NUM_BUFFERS
};


class VulkanFontDraw : public VulkanApp
{
public:
	VulkanFontDraw() {}
	virtual ~VulkanFontDraw() override;
	//bool initApp(const std::string &fontFilename);
	virtual bool init(const char* windowStr, int screenWidth, int screenHeight) override;
	virtual void run() override;

	bool createPipelines();

public:

	VkShaderModule shaderModules[NUM_SHADER_MODULES] = {};
	Buffer buffers[NUM_BUFFERS];

	std::vector<DescriptorSet> descriptorSets[NUM_SHADER_MODULES];
	PipelineWithDescriptors pipelinesWithDescriptors[NUM_PIPELINE];

	std::string fontFilename;
};


////////////////////////
//
// DEINIT
//
////////////////////////

VulkanFontDraw::~VulkanFontDraw()
{
	VkDevice device = deviceWithQueues.device;

	for (auto& pipeline : pipelinesWithDescriptors)
	{
		destroyDescriptor(device, pipeline.descriptor);
		destroyPipeline(device, pipeline.pipeline);
	}

	for (auto& buffer : buffers)
		destroyBuffer(device, buffer);

	for (auto& shaderModule : shaderModules)
		vkDestroyShaderModule(device, shaderModule, nullptr);

}

bool skipWord(const std::vector<char>& buffer, JSONBlock &block)
{
	int& index = block.currentIndex;
	int sz = block.endIndex;
	while (index < sz && !isspace(buffer[index]))
		++index;

	return index < sz;
}


bool skipWhiteSpace(const std::vector<char>& buffer, JSONBlock& block)
{
	int& index = block.currentIndex;
	int sz = block.endIndex;
	while (index < sz && isspace(buffer[index]))
		++index;

	return index < sz;
}

JSONBlock getTypeContent(const std::vector<char>& buffer, JSONBlock &block)
{
	JSONBlock result(0);
	int &index = block.currentIndex;
	int endIndex = block.endIndex;

	if(!skipWhiteSpace(buffer, block))
		return result;

	char findOpen = '[';
	char findClosed = ']';

	if (buffer[index] == '[')
	{
		findOpen = '[';
		findClosed = ']';
		result.vbracket = true;
	}
	else if (buffer[index] == '{')
	{
		findOpen = '{';
		findClosed = '}';
		result.varray = true;
	}
	else
	{
		result.vvalid = true;
		return result;
	}

	int bracketCount = 1;

	result.startIndex = index;

	++index;
	result.currentIndex = index;
	while (index < endIndex)
	{
		char c = buffer[index];
		if (c == findOpen)
		{
			++bracketCount;
		}
		else if (c == findClosed)
		{
			--bracketCount;
			if (bracketCount == 0)
			{
				result.endIndex = index;
				++index;
				break;
			}
		}
		++index;
	}
	if (skipWhiteSpace(buffer, block) && buffer[index] == ',')
		++index;
	result.vvalid = result.endIndex > result.startIndex;
	return result;
}

bool subStrCmp(std::vector<char>& buffer, const char* cmpStr, int startPos)
{
	int l = strlen(cmpStr);

	return startPos + l < (int)buffer.size() && strncmp(buffer.data() + startPos, cmpStr, l) == 0;
}
bool parseInt(std::vector<char>& buffer, JSONBlock &block, int64_t& outResult)
{
	int& index = block.currentIndex;
	int sz = block.endIndex;
	if(!skipWhiteSpace(buffer, block))
		return false;

	bool neg = false;
	if (buffer[index] == '-')
	{
		neg = true;
		index++;
	}
	outResult = 0;
	int numCount = 0;
	while (index < sz)
	{
		char c = buffer[index];
		if (c >= '0' && c <= '9')
		{
			++numCount;
			if (numCount > 18)
				return false;
			outResult = outResult * 10 + int(c - '0');
		}
		else
			break;

		++index;
	}
	if (neg)
		outResult = -outResult;
	return numCount > 0;
}

bool parseFloat(std::vector<char>& buffer, JSONBlock &block, float &outResult)
{
	int& index = block.currentIndex;
	outResult = 0.0f;
	int64_t num = 0;
	int sz = block.endIndex;
	bool neg = false;
	if (!skipWhiteSpace(buffer, block))
		return false;
	if (buffer[index] == '-')
	{
		++index;
		neg = true;
	}

	if (!parseInt(buffer, block, num))
		return false;
	outResult = num;
	if (isspace(buffer[index]))
		return true;
	
	if (index >= sz)
		return false;

	if(buffer[index] != '.')
		return false;
	++index;
	int tmpStartIndex = index;
	if (!parseInt(buffer, block, num))
		return false;
	
	if (num < 0)
		return false;
	float div = 1.0f;
	while(tmpStartIndex < index)
	{
		div *= 10.0f;
		++tmpStartIndex;
	}
	outResult += num / div;
	if (neg)
		outResult = -outResult;
	if (buffer[index] == ',')
		++index;

	return index < sz;
}

bool parseStringQuotas(std::vector<char>& buffer, JSONBlock& block, std::string& outStr)
{
	int& index = block.currentIndex;
	int sz = block.endIndex;
	if (!skipWhiteSpace(buffer, block))
		return false;

	if (buffer[index] != '"')
		return false;

	++index;

	int startIndex = index;
	while (index < sz && buffer[index] != '"')
		++index;

	if (index >= sz || startIndex == index)
		return false;

	int letters = index - startIndex;
	outStr.resize(letters);
	for (int i = 0; i < letters; ++i)
	{
		outStr[i] = buffer[index - letters + i];
	}
	++index;
	if (index < sz && buffer[index] == ',')
		++index;

	return index < sz;
}

bool parseVariableName(std::vector<char>& buffer, JSONBlock &block, std::string &outStr)
{
	if (!parseStringQuotas(buffer, block, outStr))
		return false;

	if (!skipWhiteSpace(buffer, block))
		return false;

	if (buffer[block.currentIndex] != ':')
		return false;

	++block.currentIndex;
	return skipWhiteSpace(buffer, block);
}

bool readGLTF(const char *filename)
{
	std::string fName = std::string(filename);
	std::vector<char> buffer;

	if (!loadBytes(fName, buffer))
		return false;

	struct Vertex
	{
		Vec3 pos;
		Vec3 norm;
		Vec4 color;
	};

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	struct SceneNode
	{
		std::string name;
		Quat rot;
		Vec3 trans;
		uint32_t meshIndex = ~0u;
	};

	struct BufferType
	{
		uint32_t dataCount;
		uint32_t componentType;
		// scalar = 1, vec2 = 2....
		uint32_t componentCount;

		Vec3 minValue;
		Vec3 maxValue;
	
		uint32_t bufferViewIndex;
		uint32_t bufferIndex;
		uint32_t len;
		uint32_t off;
	};


	std::vector<SceneNode> nodes;
	std::vector<BufferType> bufferTypes;
	std::vector<std::vector<uint8_t>> buffers;

	uint32_t positionIndex = ~0u;
	uint32_t normalIndex = ~0u;
	uint32_t colorIndex = ~0u;
	uint32_t indicesIndex = ~0u;


	int sz = (int)buffer.size();

	std::string s;
	s.reserve(256);
	JSONBlock tmp(0, sz);
	JSONBlock b1 = getTypeContent(buffer, tmp);
	if (!b1.isArray())
		return false;



	while (parseVariableName(buffer, b1, s))
	{
		char cc = buffer[b1.currentIndex];
		JSONBlock b2 = getTypeContent(buffer, b1);
		if (!b2.isValid())
			return false;

		if (s == "scene")
			skipWord(buffer, b1);
		else if (!(b2.isBracket() || b2.isArray()))
			return false;

		if (s == "scenes")
		{
			LOG("Scenes\n");

		}

		else if (s == "nodes" && b2.isBracket())
		{
			LOG("Nodes\n");
			
			int nodeIndex = 0;

			JSONBlock b3 = getTypeContent(buffer, b2);
			while (b3.isArray())
			{
				while (nodeIndex >= nodes.size())
					nodes.emplace_back(SceneNode());
					
				SceneNode& node = nodes[nodeIndex];
				++nodeIndex;

				while (parseVariableName(buffer, b3, s))
				{
					JSONBlock b4 = getTypeContent(buffer, b3);
					if (!b4.isValid())
						return false;

					if (s == "mesh")
					{
						int64_t ind = -1;
						if (!parseInt(buffer, b3, ind))
							return false;
						node.meshIndex = (int)ind;
					}
					else if (s == "name")
					{
						if(!parseStringQuotas(buffer, b3, node.name))
							return false;
					}
					else if (s == "rotation")
					{
						if (!parseFloat(buffer, b4, node.rot.v.x))
							return false;
						if (!parseFloat(buffer, b4, node.rot.v.y))
							return false;
						if (!parseFloat(buffer, b4, node.rot.v.z))
							return false;
						if (!parseFloat(buffer, b4, node.rot.w))
							return false;
					}
					else if (s == "translation")
					{
						if (!parseFloat(buffer, b4, node.trans.x))
							return false;
						if (!parseFloat(buffer, b4, node.trans.y))
							return false;
						if (!parseFloat(buffer, b4, node.trans.z))
							return false;

					}

					if (buffer[b3.currentIndex] == ',')
						++b3.currentIndex;
				}
				b3 = getTypeContent(buffer, b2);
			}
			// take all the nodes....

		}
		else if (s == "meshes")
		{
			LOG("Meshes\n");

		}
		else if (s == "buffers")
		{
			LOG("Buffers\n");
			while (b1.currentIndex < b1.endIndex && 
				!subStrCmp(buffer, "\"uri\" : \"data:application/octet-stream;base64,", b1.currentIndex))
				++b1.currentIndex;



			if (b1.currentIndex < sz)
			{
				std::vector<uint8_t> byteData;
				byteData.reserve(1024);
				int byteOffset = 0;
				uint8_t currentByte = 0u;
				b1.currentIndex += 46;
				while (b1.currentIndex < sz)
				{
					char c = buffer[b1.currentIndex];
					uint8_t readByte = 0u;

					if (c >= 'A' && c <= 'Z') readByte = (c - 'A');
					if (c >= 'a' && c <= 'z') readByte = (c - 'a') + 26;
					if (c >= '0' && c <= '9') readByte = (c - '0') + 52;
					if (c == '+') readByte = 62;
					if (c == '/') readByte = 63;
					if (c == '\"')
						break;

					if (byteOffset == 0)
					{
						currentByte = readByte << 2;
					}
					else if (byteOffset == 1)
					{
						currentByte |= (readByte >> 4);
						byteData.push_back(currentByte);
						currentByte = (readByte & 15) << 4;
					}
					else if (byteOffset == 2)
					{
						currentByte |= (readByte >> 2);
						byteData.push_back(currentByte);
						currentByte = (readByte & 3) << 6;
					}
					else if (byteOffset == 3)
					{
						currentByte |= (readByte);
						byteData.push_back(currentByte);
						currentByte = 0;
					}
					byteOffset = (byteOffset + 1) % 4;
					++b1.currentIndex;
				}
				if (currentByte != 0)
					byteData.push_back(currentByte);
				int index = 0;
				int rowC = 0;
				for (; index < 288; index += 12)
				{
					Vec3 v;
					memcpy(&v.x, byteData.data() + index, 12);
					printf("pos %i: x: %f, y: %f, z: %f\n", rowC, v.x, v.y, v.z);
					++rowC;
				}
				rowC = 0;
				for (; index < 288 + 288; index += 12)
				{
					Vec3 v;
					memcpy(&v.x, byteData.data() + index, 12);
					printf("norm %i: x: %f, y: %f, z: %f\n", rowC, v.x, v.y, v.z);
					++rowC;
				}
				rowC = 0;
				for (; index < 576 + 192; index += 8)
				{
					Vec2 v;
					memcpy(&v.x, byteData.data() + index, 8);
					printf("tx0 %i: x: %f, y: %f\n", rowC, v.x, v.y);
					++rowC;
				}
				rowC = 0;
				for (; index < 768 + 192; index += 8)
				{
					//09B6FF
					// normalized
					Vec4 v;

					for (int i = 0; i < 4; ++i)
					{
						uint32_t t = 0;
						memcpy(&t, byteData.data() + index + i * 2, 2);
						
						float f = float(t) / 65535.0f;
						v[i] = f;
					}
					printf("col %i: x: %f, y: %f, z: %f, w: %f\n", rowC, v.x, v.y, v.z, v.w);
					++rowC;
				}
				rowC = 0;
				for (; index < 960 + 72; index += 2)
				{
					uint16_t v;
					memcpy(&v, byteData.data() + index, 2);
					printf("ind %i: x: %i\n", rowC, v);
					++rowC;
				}
			}
		}
	}


	return true;
}

bool VulkanFontDraw::init(const char* windowStr, int screenWidth, int screenHeight)
{
	if (!VulkanApp::init(windowStr, screenWidth, screenHeight))
		return false;

	glfwSetWindowUserPointer(window, this);
	printf("gltf read success: %i\n", readGLTF("assets/models/test_gltf.gltf"));

	VkDevice device = deviceWithQueues.device;

	shaderModules[SHADER_MODULE_RENDER_QUAD_VERT] = loadShader(device, "assets/shader/vulkan_new/coloredquad.vert.spv");
	ASSERT(shaderModules[SHADER_MODULE_RENDER_QUAD_VERT]);

	shaderModules[SHADER_MODULE_RENDER_QUAD_FRAG] = loadShader(device, "assets/shader/vulkan_new/coloredquad.frag.spv");
	ASSERT(shaderModules[SHADER_MODULE_RENDER_QUAD_FRAG]);



	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	buffers[QUAD_BUFFER] = createBuffer(device, memoryProperties, 8u * 1024u * 1024u,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Quad buffer");

	buffers[INDEX_DATA_BUFFER] = createBuffer(device, memoryProperties, 32 * 1024 * 1024,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");

	// Random tag data
	//struct DemoTag { const char name[17] = "debug marker tag"; } demoTag;
	//setObjectTag(device, (uint64_t)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);


	{
		uint32_t offset = 0;
		std::vector<uint32_t> indices;
		indices.resize(6 * 10240);
		for (int i = 0; i < 10240; ++i)
		{
			indices[size_t(i) * 6 + 0] = i * 4 + 0;
			indices[size_t(i) * 6 + 1] = i * 4 + 1;
			indices[size_t(i) * 6 + 2] = i * 4 + 2;

			indices[size_t(i) * 6 + 3] = i * 4 + 0;
			indices[size_t(i) * 6 + 4] = i * 4 + 2;
			indices[size_t(i) * 6 + 5] = i * 4 + 3;
		}
		offset = uploadToScratchbuffer(scratchBuffer, (void*)indices.data(), size_t(sizeof(indices[0]) * indices.size()), offset);
		uploadScratchBufferToGpuBuffer(device, commandPool, commandBuffer, deviceWithQueues.graphicsQueue,
			buffers[INDEX_DATA_BUFFER], scratchBuffer, offset);
	}
	return true;
}


bool VulkanFontDraw::createPipelines()
{
	VkDevice device = deviceWithQueues.device;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
	//recreateSwapchainData();

	PipelineWithDescriptors& pipeline = pipelinesWithDescriptors[PIPELINE_GRAPHICS_PIPELINE];

	pipeline.descriptorSet = std::vector<DescriptorSet>(
		{
			DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u, true, &renderFrameBuffer, 0u, 64u * 1024u},
			DescriptorSet{ VK_SHADER_STAGE_ALL_GRAPHICS, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, true, &buffers[QUAD_BUFFER] },
		});
	VertexInput vertexInput;
	pipeline.pipeline = createGraphicsPipeline(
		device, renderPass, pipelineCache,
		shaderModules[SHADER_MODULE_RENDER_QUAD_VERT],
		shaderModules[SHADER_MODULE_RENDER_QUAD_FRAG],
		vertexInput, pipeline.descriptorSet, false,
		0u, VK_SHADER_STAGE_ALL_GRAPHICS);
	pipeline.descriptor = createDescriptor(device, pipeline.descriptorSet, pipeline.pipeline.descriptorSetLayout);

	return true;
}


void VulkanFontDraw::run()
{
	const u32 charCount = 128 - 32;
	std::vector<char> data;
	if (!loadBytes(fontFilename, data))
	{
		printf("Failed to load file: %s\n", fontFilename.c_str());
		return;
	}

	int32_t chosenLetter = 'a';

	std::vector<GPUVertexData> vertData;
	vertData.resize(12 * 8 * (charCount + 1) + 1);


	static constexpr float buttonSize = 20.0f;
	static constexpr float smallButtonSize = 2.0f;
	static constexpr float borderSizes = 2.0f;

	{
		float offX = (borderSizes + buttonSize) + windowWidth * 0.5f;
		float offY = (borderSizes + buttonSize) + windowHeight * 0.5f;

		GPUVertexData& vdata = vertData[0];
		vdata.color = getColor(1.0f, 0.0f, 0.0f, 1.0f);
		vdata.pixelSizeX = uint16_t(smallButtonSize) * 8 + 4;
		vdata.pixelSizeY = uint16_t(smallButtonSize) * 12 + 4;
		vdata.posX = offX;
		vdata.posY = offY;
	}

	for (int j = 0; j < 12; ++j)
	{
		for (int i = 0; i < 8; ++i)
		{
			float offX = float((i - 4) * (borderSizes + buttonSize)) + windowWidth * 0.5f;// -buttonSize * 0.5f;
			float offY = float((j - 6) * (borderSizes + buttonSize)) + windowHeight * 0.5f;// -buttonSize * 0.5f;

			GPUVertexData& vdata = vertData[i + size_t(j) * 8 + 1];
			vdata.color = 0;
			vdata.pixelSizeX = vdata.pixelSizeY = buttonSize;
			vdata.posX = offX;
			vdata.posY = offY;
		}
	}

	for (int k = 0; k < charCount; ++k)
	{
		int x = k % 8;
		int y = k / 8;
		for (int j = 0; j < 12; ++j)
		{
			for (int i = 0; i < 8; ++i)
			{
				GPUVertexData& vdata = vertData[i + size_t(j) * 8 + (size_t(k) + 1) * 8 * 12 + 1];

				float smallOffX = float(i * (smallButtonSize)) + 10.0f + float(x * 8) * smallButtonSize + x * 2;
				float smallOffY = float(j * (smallButtonSize)) + 10.0f + float(y * 12) * smallButtonSize + y * 2;

				uint32_t indx = k * 12 + j;
				bool isVisible = ((data[indx] >> i) & 1) == 1;

				vdata.color = isVisible ? ~0u : 0u;
				vdata.pixelSizeX = vdata.pixelSizeY = smallButtonSize;
				vdata.posX = smallOffX;
				vdata.posY = smallOffY;

			}
		}
	}

	char buffData[12] = {};



	////////////////////////
	//
	// MAIN LOOP START
	// UPDATING ENGINE STATE
	//
	////////////////////////

	double previousFrameTime = glfwGetTime();
	u32 framesSinceLastDelta = 0u;
	double deltaTime = 0.0;

	u32 gpuframeCount = 0u;
	double gpuTime = 0.0;
	double cpuTimeStamp = glfwGetTime();


	VkDevice device = deviceWithQueues.device;

	while (!glfwWindowShouldClose(window))
	{
		if (++framesSinceLastDelta > 10)
		{
			double newTime = glfwGetTime();
			deltaTime = (newTime - previousFrameTime) / framesSinceLastDelta;
			previousFrameTime = newTime;
			framesSinceLastDelta = 0u;
		}

		glfwPollEvents();
		MouseState mouseState = getMouseState();
		{


			if (isPressed(GLFW_KEY_LEFT))
				--chosenLetter;

			if (isPressed(GLFW_KEY_RIGHT))
				++chosenLetter;

			if (isPressed(GLFW_KEY_DOWN))
				chosenLetter += 8;

			if (isPressed(GLFW_KEY_UP))
				chosenLetter -= 8;



			bool isControlDown = keyDowns[GLFW_KEY_LEFT_CONTROL].isDown || keyDowns[GLFW_KEY_RIGHT_CONTROL].isDown;

			for (int i = 0; i < bufferedPressesCount; ++i)
			{
				if (!isControlDown && bufferedPresses[i] >= 32 && bufferedPresses[i] < 128)
				{
					chosenLetter = (int)bufferedPresses[i];
				}
			}

			if (keyDowns[GLFW_KEY_S].isDown && keyDowns[GLFW_KEY_S].pressCount > 0u && isControlDown)
				saveFontData(fontFilename, data);

			if (keyDowns[GLFW_KEY_L].isDown && keyDowns[GLFW_KEY_L].pressCount > 0u && isControlDown)
				loadBytes(fontFilename, data);

			if (keyDowns[GLFW_KEY_C].isDown && keyDowns[GLFW_KEY_C].pressCount > 0u && isControlDown)
			{
				for (int i = 0; i < 12; ++i)
				{
					uint32_t ind = (chosenLetter - 32) * 12 + i;
					buffData[i] = data[ind];
				}
			}

			if (keyDowns[GLFW_KEY_V].isDown && keyDowns[GLFW_KEY_V].pressCount > 0u && isControlDown)
			{
				for (int i = 0; i < 12; ++i)
				{
					uint32_t ind = (chosenLetter - 32) * 12 + i;
					data[ind] = char(buffData[i]);
				}
			}

			if (chosenLetter < 32)
				chosenLetter = 32;
			if (chosenLetter > 127)
				chosenLetter = 127;

			for (int j = 0; j < 12; ++j)
			{

				for (int i = 0; i < 8; ++i)
				{
					float offX = float((i - 4) * (borderSizes + buttonSize)) + windowWidth * 0.5f;
					float offY = float((j - 6) * (borderSizes + buttonSize)) + windowHeight * 0.5f;

					bool insideRect = mouseState.x > offX - (borderSizes + buttonSize) * 0.5f &&
						mouseState.x < offX + (borderSizes + buttonSize) * 0.5f &&
						mouseState.y > offY - (borderSizes + buttonSize) * 0.5f &&
						mouseState.y < offY + (borderSizes + buttonSize) * 0.5f;

					offX -= 0.5f * buttonSize;
					offY -= 0.5f * buttonSize;

					uint32_t indx = (chosenLetter - 32) * 12 + j;

					if (mouseState.leftButtonDown && insideRect)
						data[indx] |= (1 << i);
					else if (mouseState.rightButtonDown && insideRect)
						data[indx] &= ~(char(1 << i));

					bool isVisible = ((data[indx] >> i) & 1) == 1;

					vertData[i + size_t(j) * 8 + 1].color = isVisible ? ~0u : 0u;
					vertData[i + size_t(j) * 8 + 1].posX = uint16_t(offX);
					vertData[i + size_t(j) * 8 + 1].posY = uint16_t(offY);
					vertData[(size_t(indx) + 12) * 8 + i + 1].color = isVisible ? ~0u : 0u;

				}

			}
			uint32_t xOff = (chosenLetter - 32) % 8;
			uint32_t yOff = (chosenLetter - 32) / 8;

			vertData[0].posX = 10.0f + (xOff * 8) * smallButtonSize + xOff * 2 - 2;
			vertData[0].posY = 10.0f + (yOff * 12) * smallButtonSize + yOff * 2 - 2;
		}

		ASSERT(vertData.size() * sizeof(GPUVertexData) < buffers[QUAD_BUFFER].size);


		////////////////////////
		//
		// RENDER PASSES START
		// WRITING VALUES INTO
		// "CONSTANT BUFFEERS"
		//
		////////////////////////

		if (!startRender())
			continue;

		beginSingleTimeCommands(device, commandPool, commandBuffer);
		vkCmdResetQueryPool(commandBuffer, queryPool, 0, QUERY_COUNT);
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, TIME_POINTS::START_POINT);

		{
			uint32_t offset = updateRenderFrameBuffer();
			// use scratch buffer to unifrom buffer transfer
			uint32_t vertDataSize = uint32_t(vertData.size() * sizeof(GPUVertexData));
			memcpy((void*)((char*)scratchBuffer.data + offset), vertData.data(), vertDataSize);

			{
				VkBufferCopy region = { offset, 0, VkDeviceSize(vertDataSize) };
				vkCmdCopyBuffer(commandBuffer, scratchBuffer.buffer, buffers[QUAD_BUFFER].buffer, 1, &region);
			}

			VkBufferMemoryBarrier bar[]
			{
				bufferBarrier(buffers[QUAD_BUFFER].buffer, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT, vertDataSize),
			};

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, ARRAYSIZE(bar), bar, 0, nullptr);
		}



		////////////////////////
		//
		// MAIN RENDER
		//
		////////////////////////
		{
			VkImageMemoryBarrier imageBarriers[] =
			{
				imageBarrier(mainColorRenderTarget.image,
							0, VK_IMAGE_LAYOUT_UNDEFINED,
							VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),

				imageBarrier(mainDepthRenderTarget.image,
							0, VK_IMAGE_LAYOUT_UNDEFINED,
							VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
							VK_IMAGE_ASPECT_DEPTH_BIT),
			};

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
				VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, ARRAYSIZE(imageBarriers), imageBarriers);
		}

		// Drawingg
		{
			VkClearValue clearValues[2] = {};
			clearValues[0].color = VkClearColorValue{ {0.0f, 0.5f, 1.0f, 1.0f } };
			clearValues[1].depthStencil = { 0.0f, 0 };

			VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
			passBeginInfo.renderPass = renderPass;
			passBeginInfo.framebuffer = targetFB;
			passBeginInfo.renderArea.extent.width = swapchain.width;
			passBeginInfo.renderArea.extent.height = swapchain.height;
			passBeginInfo.clearValueCount = ARRAYSIZE(clearValues);
			passBeginInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewPort = { 0.0f, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.0f, 1.0f };
			VkRect2D scissors = { { 0, 0 }, { u32(swapchain.width), u32(swapchain.height) } };

			insertDebugRegion(commandBuffer, "Render", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
			vkCmdSetViewport(commandBuffer, 0, 1, &viewPort);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissors);

			// draw calls here
			// Render
			{
				bindPipelineWithDecriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinesWithDescriptors[PIPELINE_GRAPHICS_PIPELINE]);
				vkCmdBindIndexBuffer(commandBuffer, buffers[INDEX_DATA_BUFFER].buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(commandBuffer, uint32_t(vertData.size() * 6), 1, 0, 0, 0);

			}
			vkCmdEndRenderPass(commandBuffer);
		}

		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, queryPool, TIME_POINTS::DRAW_FINISHED);

		mainColorRenderTarget.accessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		mainColorRenderTarget.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		present(mainColorRenderTarget);

		////////////////////////
		//
		// END PASS, COLLECT TIMINGS
		//
		////////////////////////


		uint64_t queryResults[TIME_POINTS::NUM_TIME_POINTS];
		vkGetQueryPoolResults(device, queryPool, 0, ARRAYSIZE(queryResults), sizeof(queryResults), queryResults, sizeof(queryResults[0]), VK_QUERY_RESULT_64_BIT);


		struct TimeValues
		{
			double timeDuration[TIME_POINTS::NUM_TIME_POINTS];
		};

		VkPhysicalDeviceProperties props = {};
		vkGetPhysicalDeviceProperties(physicalDevice, &props);

		static TimeValues timeValues = {};
		for (u32 i = TIME_POINTS::NUM_TIME_POINTS - 1; i > 0; --i)
			timeValues.timeDuration[i] += (double(queryResults[i]) - double(queryResults[i - 1])) * props.limits.timestampPeriod * 1.0e-9f;

		gpuTime += (double(queryResults[TIME_POINTS::NUM_TIME_POINTS - 1]) - double(queryResults[0])) * props.limits.timestampPeriod * 1.0e-9f;

		++gpuframeCount;
		if (glfwGetTime() - cpuTimeStamp >= 1.0)
		{
			double d = 1000.0 / gpuframeCount;
			double e = gpuframeCount;
			double currTime = glfwGetTime();
			double cpuTime = currTime - cpuTimeStamp;
			cpuTimeStamp += 1.0f;

			printf("Gpu: %.3fms, cpu: %.3fms, draw: %.3fms. GpuFps:%.1f, CpuFps:%.1f\n",
				(float)(gpuTime * d), (float)(cpuTime * d),
				(float)(timeValues.timeDuration[DRAW_FINISHED] * d),
				e / gpuTime, e / cpuTime);
			gpuframeCount = 0u;

			for (u32 i = 0; i < TIME_POINTS::NUM_TIME_POINTS; ++i)
				timeValues.timeDuration[i] = 0.0;

			gpuTime = 0.0;
		}


		char str[100];
		char renderLetter = chosenLetter != 127 ? char(chosenLetter) : ' ';
		float fps = dt > 0.0 ? float(1.0 / dt) : 0.0f;
		sprintf(str, "%2.2fms, fps: %4.2f, mx: %i, my: %i, ml: %i, mr: %i, mb: %i, Letter: %c",
			float(dt * 1000.0), fps,
			mouseState.x, mouseState.y, mouseState.leftButtonDown, mouseState.rightButtonDown, mouseState.middleButtonDown,
			renderLetter);
		setTitle(str);


		std::this_thread::sleep_for(std::chrono::milliseconds(5));

	}

	VK_CHECK(vkDeviceWaitIdle(device));

}





int main(int argCount, char** argv)
{
	std::vector<char> data;
	std::string filename;
	if (argCount < 2)
	{
		filename = "assets/font/new_font.dat";
	}
	else
	{
		filename = argv[1];
	}
	VulkanFontDraw app;
	app.fontFilename = filename;
	if (app.init("Vulkan, draw font", SCREEN_WIDTH, SCREEN_HEIGHT)
		&& app.createGraphics() && app.createPipelines())
	{
		app.run();
	}

	return 0;
}