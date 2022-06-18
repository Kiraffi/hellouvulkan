#pragma once

#include <vulkan/vulkan_core.h>
#include <string_view>

#include <myvulkan/vulkanglobal.h>

struct Shader
{
    VkShaderModule module = VK_NULL_HANDLE;

    VkShaderStageFlags stage;
    VkDescriptorType resourceTypes[64] = {};
    uint64_t resourceMask = 0;
};

enum class ShaderType : uint8_t
{
    Basic3DFrag,
    Basic3DVert,

    LineVert,

    ColoredQuadFrag,
    ColoredQuadVert,

    SpaceShip2DModelFrag,
    SpaceShip2DModelVert,
    TexturedQuadFrag,
    TexturedQuadVert,

    ComputeTestComp,


    LightingShader,
    TonemapShader,

    ConvertFromRGBAS16,

    NumShaders,
};


const Shader& getShader(ShaderType shaderType, uint32_t permutationIndex = 0u);
bool loadShaders();
void deleteLoadedShaders();

bool loadShader(std::string_view filename, Shader& outShader);
void destroyShader(Shader& shaderModule);

bool createDescriptor(Pipeline &pipeline);
bool setBindDescriptorSet(const PodVector<DescriptorSetLayout>& descriptors,
    const Vector<PodVector<DescriptorInfo>>& descriptorInfos, const PodVector<VkDescriptorSet> &descriptorSet);

