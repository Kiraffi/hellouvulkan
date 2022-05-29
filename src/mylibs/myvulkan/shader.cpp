#include "shader.h"

#include <spirv_cross/spirv.h>

#include <container/podvector.h>
#include <container/vector.h>
#include <core/general.h>
#include <core/mytypes.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkglob.h>

struct GlobalShaders
{
    PodVector<Shader> shaders;
};
static GlobalShaders *globShaders = nullptr;

// spir-v specs, 1.6 https://www.khronos.org/registry/SPIR-V/specs/unified1/SPIRV.pdf
static bool parseShaderCode(const VkShaderModuleCreateInfo &info, std::string_view filename, Shader& inOutShader)
{
    ASSERT(info.pCode);
    ASSERT(info.codeSize > 0);
    if (info.codeSize == 0 || info.pCode == nullptr)
        return false;

    const uint32_t* code = info.pCode;
    uint32_t codeSize = info.codeSize / sizeof(uint32_t);

    ASSERT(code[0] == SpvMagicNumber);
    if (code[0] != SpvMagicNumber)
        return false;

    struct Ops
    {
        uint32_t opCode;
        uint32_t typeId;
        uint32_t storageClass;
        uint32_t binding;
        uint32_t set;
    };
    uint32_t bounds = code[3];
    PodVector<Ops> ops;
    ops.resize(bounds, Ops{});

    uint32_t lineNumber = 0u;
    const uint32_t* codePtr = code + 5;
    PodVector<const uint32_t*> codeStarts;

    while (codePtr < code + codeSize)
    {
        codeStarts.push_back(codePtr);
        uint16_t words = uint16_t(*codePtr >> 16);
        uint16_t opCode = uint16_t(*codePtr & 0xffff);
        ASSERT(words);
        ASSERT(codePtr + words - 1 < code + codeSize);

        switch (opCode)
        {
            case SpvOpEntryPoint:
            {
                ASSERT(words >= 2);
                switch (codePtr[1])
                {
                    case SpvExecutionModelVertex:
                        inOutShader.stage = VK_SHADER_STAGE_VERTEX_BIT;
                        break;
                    case SpvExecutionModelFragment:
                        inOutShader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                        break;
                    case SpvExecutionModelGLCompute:
                        inOutShader.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                        break;
                    default:
                        ASSERT(0 && "Unknown shader stage!");
                        return false;
                }
                break;
            }
            case SpvOpDecorate:
            {
                ASSERT(words >= 3);

                uint32_t id = codePtr[1];
                ASSERT(id < bounds);

                switch (codePtr[2])
                {
                case SpvDecorationDescriptorSet:
                    ASSERT(words == 4);
                    ops[id].set = codePtr[3];
                    break;
                case SpvDecorationBinding:
                    ASSERT(words == 4);
                    ops[id].binding = codePtr[3];
                    break;
                }
                break;
            }
            case SpvOpTypeStruct:
            case SpvOpTypeImage:
            case SpvOpTypeSampler:
            case SpvOpTypeSampledImage:
            {
                ASSERT(words >= 2);

                uint32_t id = codePtr[1];
                ASSERT(id < bounds);

                ASSERT(ops[id].opCode == 0);
                ops[id].opCode = opCode;
                break;
            }

            case SpvOpTypePointer:
            {
                ASSERT(words == 4);

                uint32_t id = codePtr[1];
                ASSERT(id < bounds);

                ASSERT(ops[id].opCode == 0);
                ops[id].opCode = opCode;
                ops[id].typeId = codePtr[3];
                ops[id].storageClass = codePtr[2];

                break;
            }
            case SpvOpVariable:
            {
                ASSERT(words >= 4);

                uint32_t id = codePtr[2];
                ASSERT(id < bounds);

                ASSERT(ops[id].opCode == 0);
                ops[id].opCode = opCode;
                ops[id].typeId = codePtr[1];
                ops[id].storageClass = codePtr[3];
                break;
            }
        }
        codePtr += words;
    }
    ASSERT(code + codeSize == codePtr);

    for (const auto& op : ops)
    {
        if (op.opCode != SpvOpVariable)
            continue;

        switch (op.storageClass)
        {
            case SpvStorageClassUniform:
            case SpvStorageClassUniformConstant:
            case SpvStorageClassStorageBuffer:
            {
                ASSERT(op.set == 0);
                ASSERT(op.binding < 64);
                ASSERT(ops[op.typeId].opCode == SpvOpTypePointer);

                ASSERT((inOutShader.resourceMask & (1 << op.binding)) == 0);

                uint32_t typeKind = ops[ops[op.typeId].typeId].opCode;
                switch (typeKind)
                {
                    case SpvOpTypeStruct:
                        inOutShader.resourceTypes[op.binding] = op.storageClass == SpvStorageClassStorageBuffer ?
                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        inOutShader.resourceMask |= 1 << op.binding;
                        break;
                    case SpvOpTypeImage:
                        inOutShader.resourceTypes[op.binding] = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        inOutShader.resourceMask |= 1 << op.binding;
                        break;
                    case SpvOpTypeSampler:
                        inOutShader.resourceTypes[op.binding] = VK_DESCRIPTOR_TYPE_SAMPLER;
                        inOutShader.resourceMask |= 1 << op.binding;
                        break;
                    case SpvOpTypeSampledImage:
                        inOutShader.resourceTypes[op.binding] = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        inOutShader.resourceMask |= 1 << op.binding;
                        break;
                    default:
                        ASSERT(!"Unknown resource type");
                }
                break;
            }
        }
    }

    return true;
}

bool loadShader(std::string_view filename, Shader &outShader)
{
    PodVector<char> buffer;
    VkShaderModule shaderModule = 0;
    if (loadBytes(filename, buffer))
    {
        ASSERT(buffer.size() % 4 == 0);

        VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = buffer.size();
        createInfo.pCode = reinterpret_cast<uint32_t*>(buffer.data());
        VK_CHECK(vkCreateShaderModule(vulk->device, &createInfo, nullptr, &shaderModule));
        ASSERT(shaderModule);

        outShader.module = shaderModule;
        if (shaderModule)
            parseShaderCode(createInfo, filename, outShader);
    }
    return outShader.module != nullptr;
}


void destroyShader(Shader& shader)
{
    if (shader.module)
        vkDestroyShaderModule(vulk->device, shader.module, nullptr);
    shader.module = nullptr;
}


const Shader& getShader(ShaderType shaderType)
{
    ASSERT(uint32_t(shaderType) < uint32_t(ShaderType::NumShaders));
    ASSERT(uint32_t(shaderType) < globShaders->shaders.size());
    return globShaders->shaders[uint32_t(shaderType)];
}

bool loadShaders()
{
    globShaders = new GlobalShaders();
    globShaders->shaders.resize(uint8_t(ShaderType::NumShaders));
    if (!loadShader("assets/shader/vulkan_new/basic3d.frag.spv", globShaders->shaders[uint32_t(ShaderType::Basic3DFrag)])) return false;
    if (!loadShader("assets/shader/vulkan_new/basic3d.vert.spv", globShaders->shaders[uint32_t(ShaderType::Basic3DVert)])) return false;
    if (!loadShader("assets/shader/vulkan_new/basic3d_animated.vert.spv", globShaders->shaders[uint32_t(ShaderType::Basic3DAnimatedVert)])) return false;

    if (!loadShader("assets/shader/vulkan_new/coloredquad.frag.spv", globShaders->shaders[uint32_t(ShaderType::ColoredQuadFrag)])) return false;
    if (!loadShader("assets/shader/vulkan_new/coloredquad.vert.spv", globShaders->shaders[uint32_t(ShaderType::ColoredQuadVert)])) return false;

    if (!loadShader("assets/shader/vulkan_new/space_ship_2d_model.frag.spv", globShaders->shaders[uint32_t(ShaderType::SpaceShip2DModelFrag)])) return false;
        if (!loadShader("assets/shader/vulkan_new/space_ship_2d_model.vert.spv", globShaders->shaders[uint32_t(ShaderType::SpaceShip2DModelVert)])) return false;

    if (!loadShader("assets/shader/vulkan_new/texturedquad.frag.spv", globShaders->shaders[uint32_t(ShaderType::TexturedQuadFrag)])) return false;
        if (!loadShader("assets/shader/vulkan_new/texturedquad.vert.spv", globShaders->shaders[uint32_t(ShaderType::TexturedQuadVert)])) return false;

    if (!loadShader("assets/shader/vulkan_new/compute_test.comp.spv", globShaders->shaders[uint32_t(ShaderType::ComputeTestComp)])) return false;
    return true;
}

void deleteLoadedShaders()
{
    for (uint32_t i = 0; i < uint32_t(ShaderType::NumShaders); ++i)
        destroyShader(globShaders->shaders[i]);
    delete globShaders;
    globShaders = nullptr;
}



bool createDescriptor(Pipeline &pipeline)
{
    destroyDescriptor(pipeline.descriptor);

    if (pipeline.descriptorSetLayouts.size() == 0)
        return true;

    PodVector<VkDescriptorPoolSize> poolSizes(pipeline.descriptorSetLayouts.size());

    for (uint32_t i = 0; i < pipeline.descriptorSetLayouts.size(); ++i)
    {
        poolSizes[i].type = pipeline.descriptorSetLayouts[i].descriptorType;
        poolSizes[i].descriptorCount = 1;
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = uint32_t(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1; // NOTE ????

    VK_CHECK(vkCreateDescriptorPool(vulk->device, &poolInfo, nullptr, &pipeline.descriptor.pool));
    if (!pipeline.descriptor.pool)
        return false;
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pipeline.descriptor.pool;
    allocInfo.descriptorSetCount = 1u;
    allocInfo.pSetLayouts = &pipeline.descriptorSetLayout;

    VkDescriptorSet descriptorSet = 0;
    VK_CHECK(vkAllocateDescriptorSets(vulk->device, &allocInfo, &descriptorSet));
    ASSERT(descriptorSet);

    pipeline.descriptor.descriptorSet = descriptorSet;


    return true;
}

bool setBindDescriptorSet(const PodVector<DescriptorSetLayout>& descriptors,
    const PodVector<DescriptorInfo>& descriptorInfos, VkDescriptorSet descriptorSet)
{
    uint32_t writeDescriptorCount = (uint32_t)descriptorInfos.size();
    if (writeDescriptorCount < 1u)
        return false;

    PodVector<VkWriteDescriptorSet> writeDescriptorSets(writeDescriptorCount);
    PodVector<VkDescriptorBufferInfo> bufferInfos(writeDescriptorCount);

    PodVector<VkDescriptorImageInfo> imageInfos(writeDescriptorCount);

    uint32_t writeIndex = 0u;
    uint32_t bufferCount = 0u;
    uint32_t imageCount = 0u;
    for (uint32_t i = 0; i < descriptorInfos.size(); ++i)
    {
        if (descriptorInfos[i].type == DescriptorInfo::DescriptorType::BUFFER)
        {
            const VkDescriptorBufferInfo& bufferInfo = descriptorInfos[i].bufferInfo;
            ASSERT(bufferInfo.buffer);
            ASSERT(bufferInfo.range > 0u);
            bufferInfos[bufferCount] = bufferInfo;

            VkWriteDescriptorSet descriptor { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            descriptor.dstSet = descriptorSet;
            descriptor.dstArrayElement = 0;
            descriptor.descriptorType = descriptors[i].descriptorType;
            descriptor.dstBinding = descriptors[i].bindingIndex;
            descriptor.pBufferInfo = &bufferInfos[bufferCount];
            descriptor.descriptorCount = 1;

            writeDescriptorSets[writeIndex] = descriptor;

            ++bufferCount;
            ++writeIndex;
        }
        else if (descriptorInfos[i].type == DescriptorInfo::DescriptorType::IMAGE)
        {
            const VkDescriptorImageInfo& imageInfo = descriptorInfos[i].imageInfo;
            ASSERT(imageInfo.sampler || descriptors[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            ASSERT(imageInfo.imageView);
            ASSERT(imageInfo.imageLayout);

            imageInfos[imageCount] = imageInfo;

            VkWriteDescriptorSet descriptor{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            descriptor.dstSet = descriptorSet;
            descriptor.dstArrayElement = 0;
            descriptor.descriptorType = descriptors[i].descriptorType;
            descriptor.dstBinding = descriptors[i].bindingIndex;
            descriptor.pImageInfo = &imageInfos[imageCount];
            descriptor.descriptorCount = 1;

            writeDescriptorSets[writeIndex] = descriptor;

            ++writeIndex;
            ++imageCount;
        }
        else
        {
            ASSERT(true);
            return false;
        }
    }

    if (writeDescriptorSets.size() > 0)
    {
        vkUpdateDescriptorSets(vulk->device, uint32_t(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
    return true;
}



