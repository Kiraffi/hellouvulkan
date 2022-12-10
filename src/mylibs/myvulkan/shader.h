#pragma once

#include <vulkan/vulkan_core.h>

#include <myvulkan/vulkanglobal.h>

struct Shader
{
    VkShaderModule module = VK_NULL_HANDLE;

    VkShaderStageFlags stage;
    VkDescriptorType resourceTypes[64] = {};
    u64 resourceMask = 0;
};

enum class ShaderType : u8
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

class VulkanShader
{
public:

    static const Shader& getShader(ShaderType shaderType, u32 permutationIndex = 0u);
    static bool loadShaders();
    static void deleteLoadedShaders();

    static bool createDescriptor(Pipeline &pipeline);
    static bool updateBindDescriptorSet(const Pipeline &pipeline);
    static bool updateBindDescriptorSet(
        const PodVector<DescriptorSetLayout> &descriptors,
        const Vector<PodVector<DescriptorInfo>>& descriptorInfos,
        const PodVector<VkDescriptorSet> &descriptorSet);

};