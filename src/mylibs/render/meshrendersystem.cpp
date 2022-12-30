#include "meshrendersystem.h"

#include <container/arraysliceview.h>
#include <container/podvector.h>
#include <container/podvectortypedefine.h>
#include <container/string.h>
#include <container/vector.h>

#include <core/general.h>
#include <core/nullable.h>
#include <core/timer.h>

#include <math/vector3_inline_functions.h>
#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>


#include <scene/scene.h>

#include <stb_image.h>

static void sMeshRenderSystemRender(bool isShadowOnly);


struct MeshRenderSystemData
{

    // maybe there should be something that indicates its animated model or not?
    struct ModelData
    {
        uint32_t m_indiceStart = 0u;
        uint32_t m_indices = 0u;
        uint32_t m_vertexStart = 0u;
        uint32_t m_vertices = 0u;
    };

    Buffer m_vertexBuffer;
    Buffer m_animationVertexBuffer;
    Buffer m_indexDataBuffer;

    Buffer m_modelRenderMatricesBuffer[VulkanGlobal::FramesInFlight];
    Buffer m_modelBoneRenderMatricesBuffer[VulkanGlobal::FramesInFlight];
    Buffer m_modelRenderBoneStartIndexBuffer[VulkanGlobal::FramesInFlight];

    Image m_paletteImage;

    Pipeline m_meshRenderGraphicsPipeline[4];

    PodVector<ModelData> m_models;

    // these 3 probably should belong somewhere else, since they depend on scenedata, if wanting to have render to texture...
    // maybe MeshRenderScene
    Vector<PodVector< uint32_t >> m_modelRenderBoneStartIndices;
    Vector<PodVector< Mat3x4 >> m_modelRenderMatrices;
    Vector<PodVector< Mat3x4 >> m_animatedModelRenderMatrices;
    PodVector< Mat3x4 > m_boneAnimatedModelRenderMatrices;

    uint32_t m_indicesCount = 0u;
    uint32_t m_verticesCount = 0u;
    uint32_t m_animatedVerticesCount = 0u;
};

static Nullable<MeshRenderSystemData> s_meshRenderSystemData;

void MeshRenderSystem::deinit()
{
    if(s_meshRenderSystemData.get())
    {
        VulkanResources::destroyImage(s_meshRenderSystemData.get()->m_paletteImage);

        for (u32 i = 0; i < ARRAYSIZES(s_meshRenderSystemData.get()->m_meshRenderGraphicsPipeline); ++i)
            MyVulkan::destroyPipeline(s_meshRenderSystemData.get()->m_meshRenderGraphicsPipeline[i]);

        VulkanResources::destroyBuffer(s_meshRenderSystemData.get()->m_vertexBuffer);
        VulkanResources::destroyBuffer(s_meshRenderSystemData.get()->m_animationVertexBuffer);
        VulkanResources::destroyBuffer(s_meshRenderSystemData.get()->m_indexDataBuffer);

        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            VulkanResources::destroyBuffer(s_meshRenderSystemData.get()->m_modelRenderMatricesBuffer[i]);
            VulkanResources::destroyBuffer(s_meshRenderSystemData.get()->m_modelBoneRenderMatricesBuffer[i]);
            VulkanResources::destroyBuffer(s_meshRenderSystemData.get()->m_modelRenderBoneStartIndexBuffer[i]);
        }
        s_meshRenderSystemData.destroy();
    }
}


bool MeshRenderSystem::init()
{
    s_meshRenderSystemData.create();

    i32 imageWidth;
    i32 imageHeight;
    i32 imageComponents;

    unsigned char* image = stbi_load("assets/textures/palette.png", &imageWidth, &imageHeight, &imageComponents, STBI_rgb_alpha);

    if (image == nullptr)
    {
        printf("Failed to load palette file\n");
        return false;
    }

    if (!VulkanResources::createImage(imageWidth, imageHeight, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "PaletteTexture", s_meshRenderSystemData.get()->m_paletteImage))
    {
        printf("Failed to create image for text!\n");
        return false;
    }

    VulkanResources::updateImageWithData(imageWidth, imageHeight, 4u,
        s_meshRenderSystemData.get()->m_paletteImage,
        (u32)imageWidth * imageHeight * 4u, (void*)image);
    delete image;
    image = nullptr;

    s_meshRenderSystemData.get()->m_vertexBuffer = VulkanResources::createBuffer(32u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex buffer");

    s_meshRenderSystemData.get()->m_animationVertexBuffer = VulkanResources::createBuffer(32u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Animation vertex buffer");

    s_meshRenderSystemData.get()->m_indexDataBuffer = VulkanResources::createBuffer(32 * 1024 * 1024,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");

    for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
    {
        s_meshRenderSystemData.get()->m_modelRenderMatricesBuffer[i] = VulkanResources::createBuffer(4u * 1024u * 1024u,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Render matrices buffer");

        s_meshRenderSystemData.get()->m_modelRenderBoneStartIndexBuffer[i] = VulkanResources::createBuffer(2u * 1024u * 1024u,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Render model bone start indices");


        s_meshRenderSystemData.get()->m_modelBoneRenderMatricesBuffer[i] = VulkanResources::createBuffer(16u * 1024u * 1024u,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Render bone matrices buffer");


    }

    s_meshRenderSystemData.get()->m_modelRenderMatrices.resize(u32(EntityType::NUM_OF_ENTITY_TYPES));
    s_meshRenderSystemData.get()->m_animatedModelRenderMatrices.resize(u32(EntityType::NUM_OF_ENTITY_TYPES));
    s_meshRenderSystemData.get()->m_modelRenderBoneStartIndices.resize(u32(EntityType::NUM_OF_ENTITY_TYPES));
    s_meshRenderSystemData.get()->m_boneAnimatedModelRenderMatrices.reserve(65536);
    s_meshRenderSystemData.get()->m_models.resize(u32(EntityType::NUM_OF_ENTITY_TYPES));

    // pipelines.
    for(u32 i = 0; i < ARRAYSIZES(s_meshRenderSystemData.get()->m_meshRenderGraphicsPipeline); ++i)
    {
        Pipeline &pipeline = s_meshRenderSystemData.get()->m_meshRenderGraphicsPipeline[i];
        PodVector<RenderTarget> renderTargets;
        //bool animationRender = (i & 2) == 0;
        bool depthOnlyRender = (i & 1) != 0;

        if(!depthOnlyRender)
        {
            renderTargets = {
                RenderTarget{ .format = vulk->defaultColorFormat },
                RenderTarget{ .format = VK_FORMAT_R16G16B16A16_SNORM },
            };
        }
        String name = "Mesh system render - ";
        name.append(i);

        pipeline.descriptorSetBinds.resize(VulkanGlobal::FramesInFlight);
        pipeline.descriptor.descriptorSets.resize(VulkanGlobal::FramesInFlight);

        if(!depthOnlyRender)
        {
            pipeline.renderPass = MyVulkan::createRenderPass(
                {
                    RenderTarget{ .format = vulk->defaultColorFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR },
                    RenderTarget{ .format = VK_FORMAT_R16G16B16A16_SNORM, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR }
                },
                RenderTarget{ .format = vulk->depthFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR } );
        }
        else
        {
            pipeline.renderPass = MyVulkan::createRenderPass( {},
                RenderTarget{ .format = vulk->depthFormat, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR });
        }
        ASSERT(pipeline.renderPass);
        if(!pipeline.renderPass)
            return false;

        VkPipelineColorBlendAttachmentState rgbaAtt{ .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        PodVector< VkPipelineColorBlendAttachmentState > blends;
        if(!depthOnlyRender)
        {
            blends = { rgbaAtt, rgbaAtt };
        }

        if(!MyVulkan::createGraphicsPipeline(
            VulkanShader::getShader(ShaderType::Basic3DVert, i),
            depthOnlyRender ? Shader{} : VulkanShader::getShader(ShaderType::Basic3DFrag),
            blends,
            { .depthTarget = RenderTarget{ .format = vulk->depthFormat }, .useDepthTest = true, .writeDepth = true },
            pipeline, name.getStr()))
        {
            printf("Failed to create graphics pipeline\n");
        }

        for(u32 i = 0; i < VulkanGlobal::FramesInFlight; ++i)
        {
            pipeline.descriptorSetBinds[i] = PodVector<DescriptorInfo>{
                DescriptorInfo(vulk->renderFrameBufferHandle[i]),

                DescriptorInfo(s_meshRenderSystemData.get()->m_modelRenderMatricesBuffer[i]),
                DescriptorInfo(s_meshRenderSystemData.get()->m_modelBoneRenderMatricesBuffer[i]),

                DescriptorInfo(s_meshRenderSystemData.get()->m_vertexBuffer),
                DescriptorInfo(s_meshRenderSystemData.get()->m_animationVertexBuffer),
                DescriptorInfo(s_meshRenderSystemData.get()->m_modelRenderBoneStartIndexBuffer[i]),

            };
            if(!depthOnlyRender)
                pipeline.descriptorSetBinds[i].push_back(
                    DescriptorInfo(s_meshRenderSystemData.get()->m_paletteImage.imageView,
                    s_meshRenderSystemData.get()->m_paletteImage.layout, vulk->globalTextureSampler));
        }

        if (!VulkanShader::updateBindDescriptorSet(pipeline))
            return false;
    }

    return true;
}

bool MeshRenderSystem::addModel(const GltfModel& model, EntityType entityType)
{
    // Could be packed better
    struct RenderModel
    {
        Vec3 position;
        u32 color = ~0u;

        Vec2 uv;

        u16 normal[3];
        // 1, use vertexcolor, 2, use uvs
        u16 attributes = 0u;
    };
    struct AnimatedRenderModel
    {
        RenderModel model;
        u16 boneWeights[4];
        u32 boneIndices = 0;
        u32 padding;
    };

    for(u32 j = 0; j < model.modelMeshes.size(); ++j)
    {
        const auto &mesh = model.modelMeshes[j];
        bool isAnimated = mesh.animationVertices.size() > 0;

        u32 newVertices = mesh.vertices.size();
        u32 newIndices = mesh.indices.size();


        PodVector<RenderModel> renderModel;
        PodVector<AnimatedRenderModel> animatedRenderModel;

        if(isAnimated)
            animatedRenderModel.resize(newVertices);
        else
            renderModel.resize(newVertices);

        for(u32 i = 0; i < newVertices; ++i)
        {
            RenderModel &rendModel = isAnimated ? animatedRenderModel[i].model : renderModel[i];

            rendModel.position = mesh.vertices[i].pos;
            Vec3 norm = (mesh.vertices[i].norm + Vec3(1.0f, 1.0f, 1.0f)) * 0.5f * 65535.0f;
            rendModel.normal[0] = u16(norm.x);
            rendModel.normal[1] = u16(norm.y);
            rendModel.normal[2] = u16(norm.z);

            if(i < mesh.vertexColors.size())
            {
                rendModel.color = getColor(mesh.vertexColors[i]);
                rendModel.attributes |= 1;
            }

            if(i < mesh.vertexUvs.size())
            {
                rendModel.uv = mesh.vertexUvs[i];
                rendModel.attributes |= 2;
            }

            if(isAnimated)
            {
                AnimatedRenderModel &animatedModel = animatedRenderModel[i];
                const auto &animationVertex = mesh.animationVertices[i];
                Vec4 weights = (animationVertex.weights) * 65535.0f;
                animatedModel.boneWeights[0] = u16(weights.x);
                animatedModel.boneWeights[1] = u16(weights.y);
                animatedModel.boneWeights[2] = u16(weights.z);
                animatedModel.boneWeights[3] = u16(weights.w);

                //printf("[%u, %u, %u, %u]\n", animationVertex.boneIndices[0], animationVertex.boneIndices[1], animationVertex.boneIndices[2], animationVertex.boneIndices[3]);

                animatedModel.boneIndices =
                    (animationVertex.boneIndices[0] << 0) |
                    (animationVertex.boneIndices[1] << 8) |
                    (animationVertex.boneIndices[2] << 16) |
                    (animationVertex.boneIndices[3] << 24);
            }
        }

        MyVulkan::beginSingleTimeCommands();

        VulkanResources::addToCopylist(
            sliceFromPodVectorBytes(mesh.indices),
            s_meshRenderSystemData.get()->m_indexDataBuffer,
            s_meshRenderSystemData.get()->m_indicesCount * sizeof(u32));
        if(isAnimated)
            VulkanResources::addToCopylist(
                sliceFromPodVectorBytes(animatedRenderModel),
                s_meshRenderSystemData.get()->m_animationVertexBuffer,
                s_meshRenderSystemData.get()->m_animatedVerticesCount * sizeof(AnimatedRenderModel));
        else
            VulkanResources::addToCopylist(
                sliceFromPodVectorBytes(renderModel),
                s_meshRenderSystemData.get()->m_vertexBuffer,
                s_meshRenderSystemData.get()->m_verticesCount * sizeof(RenderModel));
        VulkanResources::flushBarriers(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

        MyVulkan::endSingleTimeCommands();


        s_meshRenderSystemData.get()->m_models[u32(entityType)] = MeshRenderSystemData::ModelData{
            .m_indiceStart = s_meshRenderSystemData.get()->m_indicesCount, .m_indices = newIndices,
            .m_vertexStart = isAnimated
                ? s_meshRenderSystemData.get()->m_animatedVerticesCount
                : s_meshRenderSystemData.get()->m_verticesCount, .m_vertices = newVertices,
        };


        if(isAnimated)
            s_meshRenderSystemData.get()->m_animatedVerticesCount += newVertices;
        else
            s_meshRenderSystemData.get()->m_verticesCount += newVertices;
        s_meshRenderSystemData.get()->m_indicesCount += newIndices;
    }
    return true;
}

void MeshRenderSystem::clear()
{
    for (auto& matrices : s_meshRenderSystemData.get()->m_modelRenderMatrices)
        matrices.clear();
    for (auto& matrices : s_meshRenderSystemData.get()->m_animatedModelRenderMatrices)
        matrices.clear();
    for(auto &boneAmounts : s_meshRenderSystemData.get()->m_modelRenderBoneStartIndices)
        boneAmounts.clear();
    s_meshRenderSystemData.get()->m_boneAnimatedModelRenderMatrices.clear();
}

bool MeshRenderSystem::addModelToRender(u32 modelIndex, const Mat3x4& renderMatrix, const Mat3x4 &renderNormalMatrix,
    const PodVector<Mat3x4>& boneAndBoneNormalMatrices)
{
    if (modelIndex >= s_meshRenderSystemData.get()->m_models.size())
        return false;

    if (boneAndBoneNormalMatrices.size() > 0u)
    {
        s_meshRenderSystemData.get()->m_animatedModelRenderMatrices[modelIndex].pushBack(renderMatrix);
        s_meshRenderSystemData.get()->m_animatedModelRenderMatrices[modelIndex].pushBack(renderNormalMatrix);

        s_meshRenderSystemData.get()->m_modelRenderBoneStartIndices[modelIndex].pushBack(
            s_meshRenderSystemData.get()->m_boneAnimatedModelRenderMatrices.size());
        s_meshRenderSystemData.get()->m_boneAnimatedModelRenderMatrices.pushBack(boneAndBoneNormalMatrices);
    }
    else
    {
        s_meshRenderSystemData.get()->m_modelRenderMatrices[modelIndex].push_back(renderMatrix);
        s_meshRenderSystemData.get()->m_modelRenderMatrices[modelIndex].push_back(renderNormalMatrix);
    }

    return true;
}

bool MeshRenderSystem::prepareToRender()
{
    //ScopedTimer sc("MeshRenderSystem::prepareToRender");
    {
        PodVector<u32> startIndices;
        startIndices.reserve(65536);
        PodVector<Mat3x4> allModelRenderMatrices;
        allModelRenderMatrices.reserve(65536);
        for (const auto& vec : s_meshRenderSystemData.get()->m_animatedModelRenderMatrices)
            allModelRenderMatrices.pushBack(vec);


        for (const auto& vec : s_meshRenderSystemData.get()->m_modelRenderMatrices)
            allModelRenderMatrices.pushBack(vec);

        for(const auto & boneAmounts : s_meshRenderSystemData.get()->m_modelRenderBoneStartIndices)
        {
            for(u32 boneStartIndex : boneAmounts)
            {
                startIndices.push_back(boneStartIndex);
            }
        }
        if(startIndices.size() > 0)
            VulkanResources::addToCopylist(
                sliceFromPodVectorBytes(startIndices),
                s_meshRenderSystemData.get()->m_modelRenderBoneStartIndexBuffer[vulk->frameIndex]);

        if(allModelRenderMatrices.size() > 0)
            VulkanResources::addToCopylist(
                sliceFromPodVectorBytes(allModelRenderMatrices),
                s_meshRenderSystemData.get()->m_modelRenderMatricesBuffer[vulk->frameIndex]);
    }
    {
        if (s_meshRenderSystemData.get()->m_boneAnimatedModelRenderMatrices.size() > 0)
            VulkanResources::addToCopylist(
                sliceFromPodVectorBytes(s_meshRenderSystemData.get()->m_boneAnimatedModelRenderMatrices),
                s_meshRenderSystemData.get()->m_modelBoneRenderMatricesBuffer[vulk->frameIndex]);
    }
    return true;
}

void sMeshRenderSystemRender(bool isShadowOnly)
{
    u32 instanceStartIndex = 0u;
    u32 passIndex = isShadowOnly ? 1u : 0u;
    // draw calls here
    // Render
    static constexpr const char* debugNames[] = {
        "Animated normal render",
        "Animated depth only render",
        "NonAnimated normal render",
        "NonAnimated depth only render"
    };

    for (; passIndex < 4; passIndex += 2)
    {
        const char* debugName = debugNames[passIndex];

        MyVulkan::beginDebugRegion(debugName, Vec4(1.0f, 1.0f, 0.0f, 1.0f));
        MyVulkan::bindGraphicsPipelineWithDecriptors(
            s_meshRenderSystemData.get()->m_meshRenderGraphicsPipeline[passIndex], vulk->frameIndex);
        for (u32 modelIndex = 0u; modelIndex < s_meshRenderSystemData.get()->m_models.size(); ++modelIndex)
        {
            const MeshRenderSystemData::ModelData &modelData = s_meshRenderSystemData.get()->m_models[modelIndex];
            if(modelData.m_vertices == 0)
                continue;
            bool animationRender = (passIndex & 2) == 0;
            u32 instances = animationRender
                ? s_meshRenderSystemData.get()->m_animatedModelRenderMatrices[modelIndex].size() / 2
                : s_meshRenderSystemData.get()->m_modelRenderMatrices[modelIndex].size() / 2;
            if (instances)
            {

                vkCmdBindIndexBuffer(vulk->commandBuffer,
                    s_meshRenderSystemData.get()->m_indexDataBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(vulk->commandBuffer, modelData.m_indices, instances,
                    modelData.m_indiceStart, modelData.m_vertexStart, instanceStartIndex);
                instanceStartIndex += instances;
            }
        }
        MyVulkan::endDebugRegion();
    }
}

void MeshRenderSystem::render(const MeshRenderTargets& meshRenderTargets)
{
    MyVulkan::beginDebugRegion("Mesh rendering colors", Vec4(1.0f, 0.0f, 0.0f, 1.0f));

    static constexpr VkClearValue colorClear = { .color{0.0f, 0.5f, 1.0f, 1.0f} };
    static constexpr VkClearValue normlClear = { .color{0.0f, 0.0f, 0.0f, 0.0f} };
    static constexpr VkClearValue depthClear = { .depthStencil = { 1.0f, 0 } };
    /*
    beginRendering({
        RenderImage{.image = &meshRenderTargets.albedoImage, .clearValue = colorClear },
        RenderImage{.image = &meshRenderTargets.normalMapImage, .clearValue = normlClear }, },
        {.image = &meshRenderTargets.depthImage, .clearValue = depthClear });
     */
    MyVulkan::beginRenderPass(
        s_meshRenderSystemData.get()->m_meshRenderGraphicsPipeline[0], { colorClear, normlClear, depthClear });
    sMeshRenderSystemRender(false);

    //vkCmdEndRendering(vulk->commandBuffer);
    vkCmdEndRenderPass(vulk->commandBuffer);

    MyVulkan::endDebugRegion();
    MyVulkan::writeStamp();
}


void MeshRenderSystem::renderShadows(const MeshRenderTargets& meshRenderTargets)
{
    static constexpr VkClearValue depthClear = { .depthStencil = { 1.0f, 0 } };
    MyVulkan::beginDebugRegion("Mesh rendering depth only", Vec4(1.0f, 0.0f, 0.0f, 1.0f));

    MyVulkan::beginRenderPass(s_meshRenderSystemData.get()->m_meshRenderGraphicsPipeline[1], { depthClear });


//    beginRendering({}, { .image = &meshRenderTargets.shadowDepthImage, .clearValue = depthClear });
    sMeshRenderSystemRender(true);
//    vkCmdEndRendering(vulk->commandBuffer);
    vkCmdEndRenderPass(vulk->commandBuffer);

    MyVulkan::endDebugRegion();
    MyVulkan::writeStamp();
}

void MeshRenderSystem::setRenderTargets(const MeshRenderTargets &meshRenderTargets)
{
    PodVector<Image> renderTargetImages{ meshRenderTargets.albedoImage,
        meshRenderTargets.normalMapImage, meshRenderTargets.depthImage };
    PodVector<Image> renderShadowTargetImages{ meshRenderTargets.shadowDepthImage };

    for(u32 passIndex = 0; passIndex < 4; ++passIndex)
    {
        bool isShadow = (passIndex & 1) == 1;
        ASSERT(VulkanResources::createFramebuffer(
            s_meshRenderSystemData.get()->m_meshRenderGraphicsPipeline[passIndex],
            isShadow ? renderShadowTargetImages : renderTargetImages));
    }
}

