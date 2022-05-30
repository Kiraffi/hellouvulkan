#pragma once

#include <vulkan/vulkan_core.h>
#include <string_view>

#include <myvulkan/vulkglob.h>

struct Shader
{
    VkShaderModule module = nullptr;

    VkShaderStageFlags stage;
    VkDescriptorType resourceTypes[64] = {};
    uint64_t resourceMask = 0;
};

enum class ShaderType : uint8_t
{
    Basic3DFrag,
    Basic3DVert,
    Basic3DAnimatedVert,
    ColoredQuadFrag,
    ColoredQuadVert,

    SpaceShip2DModelFrag,
    SpaceShip2DModelVert,
    TexturedQuadFrag,
    TexturedQuadVert,

    ComputeTestComp,
    NumShaders,
};


const Shader& getShader(ShaderType shaderType);
bool loadShaders();
void deleteLoadedShaders();

bool loadShader(std::string_view filename, Shader& outShader);
void destroyShader(Shader& shaderModule);

bool createDescriptor(Pipeline &pipeline);
bool setBindDescriptorSet(const PodVector<DescriptorSetLayout>& descriptors,
    const PodVector<DescriptorInfo>& descriptorInfos, VkDescriptorSet descriptorSet);
