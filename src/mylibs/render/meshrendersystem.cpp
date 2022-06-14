#include "meshrendersystem.h"

#include <core/general.h>
#include <core/timer.h>
#include <container/arraysliceview.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>

#include <scene/scene.h>


#include <string>


#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


MeshRenderSystem::~MeshRenderSystem()
{
    destroyImage(paletteImage);

    for (uint32_t i = 0; i < ARRAYSIZES(meshRenderGraphicsPipeline); ++i)
        destroyPipeline(meshRenderGraphicsPipeline[i]);

    destroyBuffer(vertexBuffer);
    destroyBuffer(animationVertexBuffer);
    destroyBuffer(indexDataBuffer);

    destroyBuffer(modelRenderMatricesBuffer);
    destroyBuffer(modelRenderNormaMatricesBuffer);
    destroyBuffer(modelBoneRenderMatricesBuffer);
    destroyBuffer(modelRenderBoneStartIndexBuffer);
}


bool MeshRenderSystem::init()
{
    int imageWidth;
    int imageHeight;
    int imageComponents;

    unsigned char* image = stbi_load("assets/textures/palette.png", &imageWidth, &imageHeight, &imageComponents, STBI_rgb_alpha);

    if (image == nullptr)
    {
        printf("Failed to load palette file\n");
        return false;
    }

    if (!createImage(imageWidth, imageHeight, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "PaletteTexture", paletteImage))
    {
        printf("Failed to create image for text!\n");
        return false;
    }

    updateImageWithData(imageWidth, imageHeight, 4u,
        paletteImage,
        (uint32_t)imageWidth * imageHeight * 4u, (void*)image);
    delete image;
    image = nullptr;

    vertexBuffer = createBuffer(32u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex buffer");

    animationVertexBuffer = createBuffer(32u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Animation vertex buffer");

    indexDataBuffer = createBuffer(32 * 1024 * 1024,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");


    modelRenderMatricesBuffer = createBuffer(8u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Render matrices buffer");

    modelRenderNormaMatricesBuffer = createBuffer(8u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Render normal matrices buffer");


    modelBoneRenderMatricesBuffer = createBuffer(8u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Render bone matrices buffer");

    modelRenderBoneStartIndexBuffer = createBuffer(8u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Render model bone start indices");

    

    modelRenderMatrices.resize(uint32_t(EntityType::NUM_OF_ENTITY_TYPES));
    animatedModelRenderMatrices.resize(uint32_t(EntityType::NUM_OF_ENTITY_TYPES));
    modelRenderBoneStartIndices.resize(uint32_t(EntityType::NUM_OF_ENTITY_TYPES));
    boneAnimatedModelRenderMatrices.reserve(65536);
    models.resize(uint32_t(EntityType::NUM_OF_ENTITY_TYPES));

    // pipelines.
    for(uint32_t i = 0; i < ARRAYSIZES(meshRenderGraphicsPipeline); ++i)
    {
        Pipeline& pipeline = meshRenderGraphicsPipeline[i];
        PodVector<RenderTarget> renderTargets;
        //bool animationRender = (i & 2) == 0;
        bool depthOnlyRender = (i & 1) != 0;

        if (!depthOnlyRender)
        {
            renderTargets = {
                RenderTarget{.format = vulk->defaultColorFormat },
                RenderTarget{.format = VK_FORMAT_R16G16B16A16_SNORM },
            };
        }
        std::string name = "Mesh system render - ";
        name += std::to_string(i);

        if (!createGraphicsPipeline(
            getShader(ShaderType::Basic3DVert, i), depthOnlyRender ? Shader{} : getShader(ShaderType::Basic3DFrag),
            renderTargets,
            { .depthTarget = RenderTarget{.format = vulk->depthFormat }, .useDepthTest = true, .writeDepth = true },
            pipeline, name))
        {
            printf("Failed to create graphics pipeline\n");
        }


        pipeline.descriptorSetBinds = PodVector<DescriptorInfo>(
            {
                DescriptorInfo(vulk->renderFrameBufferHandle),

                DescriptorInfo(modelRenderMatricesBuffer),
                DescriptorInfo(modelBoneRenderMatricesBuffer),
                DescriptorInfo(modelRenderNormaMatricesBuffer),

                DescriptorInfo(vertexBuffer),
                DescriptorInfo(animationVertexBuffer),
                DescriptorInfo(modelRenderBoneStartIndexBuffer),

            });
        if (!depthOnlyRender)
            pipeline.descriptorSetBinds.push_back(
                DescriptorInfo(paletteImage.imageView, paletteImage.layout, vulk->globalTextureSampler));


        if (!setBindDescriptorSet(pipeline.descriptorSetLayouts, pipeline.descriptorSetBinds, pipeline.descriptor.descriptorSet))
        {
            printf("Failed to set descriptor binds!\n");
            return false;
        }
    }

    return true;
}

bool MeshRenderSystem::addModel(const GltfModel& model, EntityType entityType)
{
    bool isAnimated = model.animationVertices.size() > 0;

    uint32_t newVertices = model.vertices.size();
    uint32_t newIndices = model.indices.size();

    // Could be packed better
    struct RenderModel
    {
        Vec3 position;
        uint32_t color = ~0u;

        Vec2 uv;

        uint16_t normal[3];
        // 1, use vertexcolor, 2, use uvs
        uint16_t attributes = 0u;
    };
    struct AnimatedRenderModel
    {
        RenderModel model;
        uint16_t boneWeights[4];
        uint32_t boneIndices = 0;
        uint32_t padding;
    };

    PodVector<RenderModel> renderModel;
    PodVector<AnimatedRenderModel> animatedRenderModel;

    if(isAnimated)
        animatedRenderModel.resize(newVertices);
    else
        renderModel.resize(newVertices);

    for (uint32_t i = 0; i < newVertices; ++i)
    {
        RenderModel& rendModel = isAnimated ? animatedRenderModel[i].model : renderModel[i];

        rendModel.position = model.vertices[i].pos;
        Vec3 norm = (model.vertices[i].norm + Vec3(1.0f, 1.0f, 1.0f)) * 0.5f * 65535.0f;
        rendModel.normal[0] = uint16_t(norm.x);
        rendModel.normal[1] = uint16_t(norm.y);
        rendModel.normal[2] = uint16_t(norm.z);

        if (i < model.vertexColors.size())
        {
            rendModel.color = getColor(model.vertexColors[i]);
            rendModel.attributes |= 1;
        }

        if (i < model.vertexUvs.size())
        {
            rendModel.uv = model.vertexUvs[i];
            rendModel.attributes |= 2;
        }

        if(isAnimated)
        {
            AnimatedRenderModel &animatedModel = animatedRenderModel[i];
            const auto &animationVertex = model.animationVertices[i];
            Vec4 weights = (animationVertex.weights) * 65535.0f;
            animatedModel.boneWeights[0] = uint16_t(weights.x);
            animatedModel.boneWeights[1] = uint16_t(weights.y);
            animatedModel.boneWeights[2] = uint16_t(weights.z);
            animatedModel.boneWeights[3] = uint16_t(weights.w);

            //printf("[%u, %u, %u, %u]\n", animationVertex.boneIndices[0], animationVertex.boneIndices[1], animationVertex.boneIndices[2], animationVertex.boneIndices[3]);
            
            animatedModel.boneIndices =
                (animationVertex.boneIndices[0] << 0) |
                (animationVertex.boneIndices[1] << 8) |
                (animationVertex.boneIndices[2] << 16) |
                (animationVertex.boneIndices[3] << 24);
        }
    }

    beginSingleTimeCommands();

    addToCopylist(sliceFromPodVectorBytes(model.indices), indexDataBuffer.buffer, indicesCount * sizeof(uint32_t));
    if(isAnimated)
        addToCopylist(sliceFromPodVectorBytes(animatedRenderModel), animationVertexBuffer.buffer, animatedVerticesCount * sizeof(AnimatedRenderModel));
    else
        addToCopylist(sliceFromPodVectorBytes(renderModel), vertexBuffer.buffer, verticesCount * sizeof(RenderModel));
    flushBarriers(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    endSingleTimeCommands();

    
    models[uint32_t(entityType)] = ModelData{
        .indiceStart = indicesCount, .indices = newIndices,
        .vertexStart = isAnimated ? animatedVerticesCount : verticesCount, .vertices = newVertices,
    };


    if(isAnimated)
        animatedVerticesCount += newVertices;
    else
        verticesCount += newVertices;
    indicesCount += newIndices;
    return true;
}

void MeshRenderSystem::clear()
{
    for (auto& matrices : modelRenderMatrices)
        matrices.clear();
    for (auto& matrices : animatedModelRenderMatrices)
        matrices.clear();
    for(auto &boneAmounts : modelRenderBoneStartIndices)
        boneAmounts.clear();
    boneAnimatedModelRenderMatrices.clear();
}

bool MeshRenderSystem::addModelToRender(uint32_t modelIndex, const Mat3x4& renderMatrix, const PodVector<Mat3x4>& boneMatrices)
{
    if (modelIndex >= models.size())
        return false;

    if (boneMatrices.size() > 0u)
    {
        animatedModelRenderMatrices[modelIndex].pushBack(renderMatrix);
        modelRenderBoneStartIndices[modelIndex].pushBack(boneAnimatedModelRenderMatrices.size());
        boneAnimatedModelRenderMatrices.pushBack(boneMatrices);
    }
    else
    {
        modelRenderMatrices[modelIndex].push_back(renderMatrix);
    }

    return true;
}

bool MeshRenderSystem::prepareToRender()
{
    //ScopedTimer sc("MeshRenderSystem::prepareToRender");
    {
        PodVector<uint32_t> startIndices;
        startIndices.reserve(65536);
        PodVector<Mat3x4> allModelRenderMatrices;
        allModelRenderMatrices.reserve(65536);
        for (const auto& vec : animatedModelRenderMatrices)
            allModelRenderMatrices.pushBack(vec);


        for (const auto& vec : modelRenderMatrices)
            allModelRenderMatrices.pushBack(vec);

        for(const auto & boneAmounts : modelRenderBoneStartIndices)
        {
            for(uint32_t boneStartIndex : boneAmounts)
            {
                startIndices.push_back(boneStartIndex);
            }
        }
        if(startIndices.size() > 0)
            addToCopylist(sliceFromPodVectorBytes(startIndices), modelRenderBoneStartIndexBuffer);

        if(allModelRenderMatrices.size() > 0)
            addToCopylist(sliceFromPodVectorBytes(allModelRenderMatrices), modelRenderMatricesBuffer);
    }
    {
        if (boneAnimatedModelRenderMatrices.size() > 0)
            addToCopylist(sliceFromPodVectorBytes(boneAnimatedModelRenderMatrices), modelBoneRenderMatricesBuffer);
    }
    return true;
}

void MeshRenderSystem::render(bool isShadowOnly)
{
    uint32_t instanceStartIndex = 0u;
    uint32_t passIndex = isShadowOnly ? 1u : 0u;
    // draw calls here
    // Render

    for (; passIndex < 4; passIndex += 2)
    {
        std::string debugName;
        if (passIndex == 0)
            debugName = "Animated normal render";
        else if (passIndex == 1)
            debugName = "Animated depth only render";
        else if (passIndex == 2)
            debugName = "NonAnimated normal render";
        else if (passIndex == 3)
            debugName = "NonAnimated depth only render";

        beginDebugRegion(debugName, Vec4(1.0f, 1.0f, 0.0f, 1.0f));
        bindPipelineWithDecriptors(VK_PIPELINE_BIND_POINT_GRAPHICS, meshRenderGraphicsPipeline[passIndex]);
        for (uint32_t modelIndex = 0u; modelIndex < models.size(); ++modelIndex)
        {
            const ModelData &modelData = models[modelIndex];
            if(modelData.vertices == 0)
                continue;
            bool animationRender = (passIndex & 2) == 0;
            uint32_t instances = animationRender ? animatedModelRenderMatrices[modelIndex].size() : modelRenderMatrices[modelIndex].size();
            if (instances)
            {
                
                vkCmdBindIndexBuffer(vulk->commandBuffer, indexDataBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(vulk->commandBuffer, modelData.indices, instances,
                    modelData.indiceStart, modelData.vertexStart, instanceStartIndex);
                instanceStartIndex += instances;
            }
        }
        endDebugRegion();
    }
}

void MeshRenderSystem::render(const MeshRenderTargets& meshRenderTargets)
{
    beginDebugRegion("Mesh rendering colors", Vec4(1.0f, 0.0f, 0.0f, 1.0f));

    static constexpr VkClearValue colorClear = { .color{0.0f, 0.5f, 1.0f, 1.0f} };
    static constexpr VkClearValue normlClear = { .color{0.0f, 0.0f, 0.0f, 0.0f} };
    static constexpr VkClearValue depthClear = { .depthStencil = { 1.0f, 0 } };
    beginRendering({
        RenderImage{.image = &meshRenderTargets.albedoImage, .clearValue = colorClear },
        RenderImage{.image = &meshRenderTargets.normalMapImage, .clearValue = normlClear }, },
        {.image = &meshRenderTargets.depthImage, .clearValue = depthClear });
    render(false);
    vkCmdEndRendering(vulk->commandBuffer);

    endDebugRegion();
    writeStamp();
}


void MeshRenderSystem::renderShadows(const MeshRenderTargets& meshRenderTargets)
{
    static constexpr VkClearValue depthClear = { .depthStencil = { 1.0f, 0 } };
    beginDebugRegion("Mesh rendering depth only", Vec4(1.0f, 0.0f, 0.0f, 1.0f));

    beginRendering({}, { .image = &meshRenderTargets.shadowDepthImage, .clearValue = depthClear });
    render(true);
    vkCmdEndRendering(vulk->commandBuffer);

    endDebugRegion();
    writeStamp();
}