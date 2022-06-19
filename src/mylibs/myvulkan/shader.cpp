#include "shader.h"

#include <spirv_cross/spirv.h>

#include <container/podvector.h>
#include <container/vector.h>
#include <core/file.h>
#include <core/general.h>
#include <core/mytypes.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanglobal.h>

// std::to_string
#include <string>
#include <vector>

struct GlobalShaders
{
    std::vector<std::vector<Shader>> shaders;
};
static GlobalShaders globalShaders;

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

bool loadShader(std::string_view filename, ShaderType shaderType)
{
    if (uint32_t(shaderType) >= uint32_t(ShaderType::NumShaders))
        return false;
    uint32_t permutationIndex = 0;
    PodVector<char> buffer;

    while(true)
    {
        std::string newFilename = "assets/shader/vulkan_new/spvs/";
        newFilename += filename;
        newFilename += "_";
        newFilename += std::to_string(permutationIndex);
        newFilename += ".spv";

        if (!fileExists(newFilename))
            break;

        if (!loadBytes(newFilename, buffer))
            return false;

        ASSERT(buffer.size() % 4 == 0);
        if (buffer.size() % 4 != 0)
            return false;

        Shader shader;

        VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = buffer.size();
        createInfo.pCode = reinterpret_cast<uint32_t*>(buffer.data());
        VK_CHECK(vkCreateShaderModule(vulk->device, &createInfo, nullptr, &shader.module));
        ASSERT(shader.module);
        if (!shader.module)
            return false;


        bool parseSuccess = parseShaderCode(createInfo, filename, shader);
        ASSERT(parseSuccess);
        if (!parseSuccess)
            return false;

        globalShaders.shaders[uint32_t(shaderType)].push_back(shader);

        ++permutationIndex;
    }
    bool loadSuccess = permutationIndex > 0u;
    if(!loadSuccess)
    {
        printf("Failed to load shader: %s\n", std::string(filename).c_str());
    }
    return loadSuccess;
}


void destroyShader(Shader& shader)
{
    if (shader.module)
        vkDestroyShaderModule(vulk->device, shader.module, nullptr);
    shader.module = VK_NULL_HANDLE;
}


const Shader& getShader(ShaderType shaderType, uint32_t permutationIndex)
{
    ASSERT(uint32_t(shaderType) < uint32_t(ShaderType::NumShaders));
    ASSERT(uint32_t(shaderType) < globalShaders.shaders.size());
    ASSERT(permutationIndex < globalShaders.shaders[uint32_t(shaderType)].size());
    return globalShaders.shaders[uint32_t(shaderType)][permutationIndex];
}

bool loadShaders()
{
    globalShaders.shaders.resize(uint8_t(ShaderType::NumShaders));
    if (!loadShader("basic3d.frag", ShaderType::Basic3DFrag)) return false;
    if (!loadShader("basic3d.vert", ShaderType::Basic3DVert)) return false;

    if (!loadShader("line.vert", ShaderType::LineVert)) return false;

    if (!loadShader("coloredquad.frag", ShaderType::ColoredQuadFrag)) return false;
    if (!loadShader("coloredquad.vert", ShaderType::ColoredQuadVert)) return false;

    if (!loadShader("space_ship_2d_model.frag", ShaderType::SpaceShip2DModelFrag)) return false;
    if (!loadShader("space_ship_2d_model.vert", ShaderType::SpaceShip2DModelVert)) return false;

    if (!loadShader("texturedquad.frag", ShaderType::TexturedQuadFrag)) return false;
    if (!loadShader("texturedquad.vert", ShaderType::TexturedQuadVert)) return false;



    if (!loadShader("compute_test.comp", ShaderType::ComputeTestComp)) return false;


    if (!loadShader("lighting.comp", ShaderType::LightingShader)) return false;
    if (!loadShader("tonemap.comp", ShaderType::TonemapShader)) return false;




    if (!loadShader("convertrgbas16.comp", ShaderType::ConvertFromRGBAS16)) return false;



    return true;
}

void deleteLoadedShaders()
{
    for (auto &shaderPermutations : globalShaders.shaders)
    {
        for (auto& shaderModule : shaderPermutations)
            destroyShader(shaderModule);
    }

    globalShaders.shaders.clear();
}



bool createDescriptor(Pipeline &pipeline)
{
    destroyDescriptor(pipeline.descriptor);

    if (pipeline.descriptorSetLayouts.size() == 0)
        return true;

    //... So the poolsizes needs to count maximum amount of resource per resource type...
    PodVector<VkDescriptorPoolSize> poolSizes;
    for(uint32_t j = 0; j < pipeline.descriptor.descriptorSets.size(); ++j)
    {
        for(uint32_t i = 0; i < pipeline.descriptorSetLayouts.size(); ++i)
        {
            VkDescriptorPoolSize *found = nullptr;
            for(auto &desc : poolSizes)
            {
                if(desc.type == pipeline.descriptorSetLayouts[i].descriptorType)
                {
                    found = &desc;
                    break;
                }
            }
            if(!found)
            {
                poolSizes.push_back(
                    VkDescriptorPoolSize{ .type = pipeline.descriptorSetLayouts[i].descriptorType, .descriptorCount = 0 });
                found = &poolSizes.back();
            }
            ++found->descriptorCount;
        }
    }
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = uint32_t(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = pipeline.descriptor.descriptorSets.size();
    ASSERT(poolInfo.maxSets);

    VK_CHECK(vkCreateDescriptorPool(vulk->device, &poolInfo, nullptr, &pipeline.descriptor.pool));
    if (!pipeline.descriptor.pool)
        return false;
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pipeline.descriptor.pool;
    allocInfo.descriptorSetCount = 1u;
    allocInfo.pSetLayouts = &pipeline.descriptorSetLayout;

    for(uint32_t i = 0; i < pipeline.descriptor.descriptorSets.size(); ++i)
    {
        VkDescriptorSet descriptorSet = 0;
        VK_CHECK(vkAllocateDescriptorSets(vulk->device, &allocInfo, &descriptorSet));
        ASSERT(descriptorSet);

        pipeline.descriptor.descriptorSets[i] = descriptorSet;
    }

    return true;
}

bool updateBindDescriptorSet(const Pipeline &pipeline)
{
    bool success = updateBindDescriptorSet(
        pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSets);

    return success;
}
    

bool updateBindDescriptorSet(const PodVector<DescriptorSetLayout>& descriptors,
    const Vector<PodVector<DescriptorInfo>>& descriptorSetInfos, const PodVector<VkDescriptorSet> &descriptorSets)
{
    if(descriptorSetInfos.size() == 0 || descriptorSets.size() != descriptorSetInfos.size())
    {
        ASSERT(false && "Descriptorbinds failed");
        printf("Failed to set descriptor binds!\n");
        return false;
    }


    for(uint32_t setIndex = 0; setIndex < descriptorSets.size(); ++setIndex)
    {
        const auto &descriptorInfos = descriptorSetInfos[setIndex];
        const auto &descriptorSet = descriptorSets[setIndex];
        uint32_t writeDescriptorCount = (uint32_t)descriptorInfos.size();
        if(writeDescriptorCount < 1u)
        {
            ASSERT(false && "Descriptorbinds failed");
            printf("Failed to set descriptor binds!\n");
            return false;
        }

        ASSERT(descriptors.size() == descriptorInfos.size());
        if(descriptors.size() != descriptorInfos.size())
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
        PodVector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.resize(writeDescriptorCount);

        PodVector<VkDescriptorBufferInfo> bufferInfos;
        bufferInfos.resize(writeDescriptorCount);

        PodVector<VkDescriptorImageInfo> imageInfos;
        imageInfos.resize(writeDescriptorCount);

        uint32_t writeIndex = 0u;
        uint32_t bufferCount = 0u;
        uint32_t imageCount = 0u;
        for(uint32_t i = 0; i < descriptorInfos.size(); ++i)
        {
            if(descriptorInfos[i].type == DescriptorInfo::DescriptorType::BUFFER)
            {
                const VkDescriptorBufferInfo &bufferInfo = descriptorInfos[i].bufferInfo;
                ASSERT(bufferInfo.buffer);
                ASSERT(bufferInfo.range > 0u);
                bufferInfos[bufferCount] = bufferInfo;

                VkWriteDescriptorSet descriptor{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
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
            else if(descriptorInfos[i].type == DescriptorInfo::DescriptorType::IMAGE)
            {
                const VkDescriptorImageInfo &imageInfo = descriptorInfos[i].imageInfo;
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
                printf("Failed to set descriptor binds!\n");
                return false;
            }
        }

        if(writeDescriptorSets.size() > 0)
        {
            vkUpdateDescriptorSets(vulk->device, uint32_t(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
        }
    }
    return true;
}



