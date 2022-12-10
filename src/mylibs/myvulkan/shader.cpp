#include "shader.h"

#include <spirv_cross/spirv.h>

#include <container/podvector.h>
#include <container/podvectortypedefine.h>
#include <container/string.h>
#include <container/stringview.h>
#include <container/vector.h>
#include <core/file.h>
#include <core/general.h>
#include <core/mytypes.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/vulkanglobal.h>


// std::to_string
//#include <vector>

struct GlobalShaders
{
    Vector<PodVector<Shader>> shaders;
};
static GlobalShaders *globalShaders = nullptr;

// spir-v specs, 1.6 https://www.khronos.org/registry/SPIR-V/specs/unified1/SPIRV.pdf
static bool parseShaderCode(const VkShaderModuleCreateInfo &info, StringView filename, Shader& inOutShader)
{
    ASSERT(info.pCode);
    ASSERT(info.codeSize > 0);
    if (info.codeSize == 0 || info.pCode == nullptr)
        return false;

    const u32* code = info.pCode;
    u32 codeSize = info.codeSize / sizeof(u32);

    ASSERT(code[0] == SpvMagicNumber);
    if (code[0] != SpvMagicNumber)
        return false;

    struct Ops
    {
        u32 opCode;
        u32 typeId;
        u32 storageClass;
        u32 binding;
        u32 set;
    };
    u32 bounds = code[3];
    PodVector<Ops> ops;
    ops.resize(bounds, Ops{});

    u32 lineNumber = 0u;
    const u32* codePtr = code + 5;
    PodVector<const u32*> codeStarts;

    while (codePtr < code + codeSize)
    {
        codeStarts.push_back(codePtr);
        u16 words = u16(*codePtr >> 16);
        u16 opCode = u16(*codePtr & 0xffff);
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

                u32 id = codePtr[1];
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

                u32 id = codePtr[1];
                ASSERT(id < bounds);

                ASSERT(ops[id].opCode == 0);
                ops[id].opCode = opCode;
                break;
            }

            case SpvOpTypePointer:
            {
                ASSERT(words == 4);

                u32 id = codePtr[1];
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

                u32 id = codePtr[2];
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

                u32 typeKind = ops[ops[op.typeId].typeId].opCode;
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

static bool sLoadShader(const char *filename, ShaderType shaderType)
{
    if (u32(shaderType) >= u32(ShaderType::NumShaders))
        return false;
    u32 permutationIndex = 0;
    PodVector<u8> buffer;

    while(true)
    {
        String newFilename = "assets/shader/vulkan_new/spvs/";
        newFilename.append(filename);
        newFilename.append("_");
        newFilename.append(permutationIndex);
        newFilename.append(".spv");

        if (!fileExists(newFilename.getStr()))
            break;

        if (!loadBytes(newFilename.getStr(), buffer.getBuffer()))
            return false;

        ASSERT(buffer.size() % 4 == 0);
        if (buffer.size() % 4 != 0)
            return false;

        Shader shader;

        VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = buffer.size();
        createInfo.pCode = reinterpret_cast<u32*>(buffer.data());
        VK_CHECK(vkCreateShaderModule(vulk->device, &createInfo, nullptr, &shader.module));
        ASSERT(shader.module);
        if (!shader.module)
            return false;


        bool parseSuccess = parseShaderCode(createInfo, filename, shader);
        ASSERT(parseSuccess);
        if (!parseSuccess)
            return false;

        globalShaders->shaders[u32(shaderType)].push_back(shader);

        ++permutationIndex;
    }
    bool loadSuccess = permutationIndex > 0u;
    if(!loadSuccess)
    {
        printf("Failed to load shader: %s\n", filename);
    }
    return loadSuccess;
}

void sDestroyShader(Shader& shader)
{
    if (shader.module)
        vkDestroyShaderModule(vulk->device, shader.module, nullptr);
    shader.module = VK_NULL_HANDLE;
}


const Shader& VulkanShader::getShader(ShaderType shaderType, u32 permutationIndex)
{
    ASSERT(u32(shaderType) < u32(ShaderType::NumShaders));
    ASSERT(u32(shaderType) < globalShaders->shaders.size());
    ASSERT(permutationIndex < globalShaders->shaders[u32(shaderType)].size());
    return globalShaders->shaders[u32(shaderType)][permutationIndex];
}

bool VulkanShader::loadShaders()
{
    ASSERT(globalShaders == nullptr);
    globalShaders = new GlobalShaders();
    globalShaders->shaders.resize(u8(ShaderType::NumShaders));
    if (!sLoadShader("basic3d.frag", ShaderType::Basic3DFrag)) return false;
    if (!sLoadShader("basic3d.vert", ShaderType::Basic3DVert)) return false;

    if (!sLoadShader("line.vert", ShaderType::LineVert)) return false;

    if (!sLoadShader("coloredquad.frag", ShaderType::ColoredQuadFrag)) return false;
    if (!sLoadShader("coloredquad.vert", ShaderType::ColoredQuadVert)) return false;

    if (!sLoadShader("space_ship_2d_model.frag", ShaderType::SpaceShip2DModelFrag)) return false;
    if (!sLoadShader("space_ship_2d_model.vert", ShaderType::SpaceShip2DModelVert)) return false;

    if (!sLoadShader("texturedquad.frag", ShaderType::TexturedQuadFrag)) return false;
    if (!sLoadShader("texturedquad.vert", ShaderType::TexturedQuadVert)) return false;



    if (!sLoadShader("compute_test.comp", ShaderType::ComputeTestComp)) return false;


    if (!sLoadShader("lighting.comp", ShaderType::LightingShader)) return false;
    if (!sLoadShader("tonemap.comp", ShaderType::TonemapShader)) return false;




    if (!sLoadShader("convertrgbas16.comp", ShaderType::ConvertFromRGBAS16)) return false;



    return true;
}

void VulkanShader::deleteLoadedShaders()
{
    for (auto &shaderPermutations : globalShaders->shaders)
    {
        for (auto& shaderModule : shaderPermutations)
            sDestroyShader(shaderModule);
    }

    globalShaders->shaders.clear();
    delete globalShaders;
    globalShaders = nullptr;
}



bool VulkanShader::createDescriptor(Pipeline &pipeline)
{
    MyVulkan::destroyDescriptor(pipeline.descriptor);

    if (pipeline.descriptorSetLayouts.size() == 0)
        return true;

    //... So the poolsizes needs to count maximum amount of resource per resource type...
    PodVector<VkDescriptorPoolSize> poolSizes;
    for(u32 j = 0; j < pipeline.descriptor.descriptorSets.size(); ++j)
    {
        for(u32 i = 0; i < pipeline.descriptorSetLayouts.size(); ++i)
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
    poolInfo.poolSizeCount = u32(poolSizes.size());
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

    for(u32 i = 0; i < pipeline.descriptor.descriptorSets.size(); ++i)
    {
        VkDescriptorSet descriptorSet = 0;
        VK_CHECK(vkAllocateDescriptorSets(vulk->device, &allocInfo, &descriptorSet));
        ASSERT(descriptorSet);

        pipeline.descriptor.descriptorSets[i] = descriptorSet;
    }

    return true;
}

bool VulkanShader::updateBindDescriptorSet(const Pipeline &pipeline)
{
    bool success = updateBindDescriptorSet(
        pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSets);

    return success;
}


bool VulkanShader::updateBindDescriptorSet(const PodVector<DescriptorSetLayout>& descriptors,
    const Vector<PodVector<DescriptorInfo>>& descriptorSetInfos, const PodVector<VkDescriptorSet> &descriptorSets)
{
    if(descriptorSetInfos.size() == 0 || descriptorSets.size() != descriptorSetInfos.size())
    {
        ASSERT(false && "Descriptorbinds failed");
        printf("Failed to set descriptor binds!\n");
        return false;
    }


    for(u32 setIndex = 0; setIndex < descriptorSets.size(); ++setIndex)
    {
        const auto &descriptorInfos = descriptorSetInfos[setIndex];
        const auto &descriptorSet = descriptorSets[setIndex];
        u32 writeDescriptorCount = (u32)descriptorInfos.size();
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

        u32 writeIndex = 0u;
        u32 bufferCount = 0u;
        u32 imageCount = 0u;
        for(u32 i = 0; i < descriptorInfos.size(); ++i)
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
            vkUpdateDescriptorSets(vulk->device, u32(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
        }
    }
    return true;
}



