#include "meshrendersystem.h"

#include <container/arraysliceview.h>

#include <myvulkan/myvulkan.h>
#include <myvulkan/shader.h>

#include <scene/scene.h>


#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

static uint32_t convertVec4ColorToU32(const Vec4& col)
{
    ASSERT(
        col.x >= 0.0f && col.x <= 1.0f &&
        col.y >= 0.0f && col.y <= 1.0f &&
        col.z >= 0.0f && col.z <= 1.0f &&
        col.w >= 0.0f && col.w <= 1.0f);

    uint32_t result = 0u;

    return (
        (uint32_t(col.x * 255.0f) << 0) +
        (uint32_t(col.y * 255.0f) << 8) +
        (uint32_t(col.z * 255.0f) << 16) +
        (uint32_t(col.w * 255.0f) << 24));
}

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
        //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Vertex buffer");

    animationVertexBuffer = createBuffer(32u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Animation vertex buffer");

    indexDataBuffer = createBuffer(32 * 1024 * 1024,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Index data buffer");


    modelRenderMatricesBuffer = createBuffer(8u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Render matrices buffer");

    modelRenderNormaMatricesBuffer = createBuffer(8u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Render normal matrices buffer");
    

    modelBoneRenderMatricesBuffer = createBuffer(8u * 1024u * 1024u,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "Uniform buffer2");
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "Render bone matrices buffer");

    modelRenderMatrices.resize(uint32_t(EntityType::NUM_OF_ENTITY_TYPES));
    animatedModelRenderMatrices.resize(uint32_t(EntityType::NUM_OF_ENTITY_TYPES));
    boneAnimatedModelRenderMatrices.resize(uint32_t(EntityType::NUM_OF_ENTITY_TYPES));


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

uint32_t MeshRenderSystem::addModel(const GltfModel& model)
{
    uint32_t newVertices = model.vertices.size();
    uint32_t newAnimationVertices = model.animationVertices.size();
    uint32_t newIndices = model.indices.size();
    // Make sure to add animated models first to keep animatedvertex index same as vertexindex.
    ASSERT(newAnimationVertices == 0 || verticesCount == animatedVerticesCount);
    uint32_t result = ~0u;


    // Could be packed better
    struct RenderModel
    {
        Vec4 position;
        Vec4 normal;
        Vec2 uv;
        uint32_t color = ~0u;

        // 1, use vertexcolor, 2, use uvs
        uint32_t attributes = 0u;
    };

    PodVector<RenderModel> renderModel;
    renderModel.resize(newVertices);

    for (uint32_t i = 0; i < newVertices; ++i)
    {
        RenderModel& rendModel = renderModel[i];
        
        rendModel.position = model.vertices[i].pos;
        rendModel.normal = model.vertices[i].norm;

        if (i < model.vertexColors.size())
        {
            rendModel.color = convertVec4ColorToU32(model.vertexColors[i]);
            rendModel.attributes |= 1;
        }

        if (i < model.vertexUvs.size())
        {
            rendModel.uv = model.vertexUvs[i];
            rendModel.attributes |= 2;
        }

    }

    beginSingleTimeCommands();

    addToCopylist(sliceFromPodVectorBytes(model.indices), indexDataBuffer.buffer, indicesCount * sizeof(uint32_t));
    addToCopylist(sliceFromPodVectorBytes(renderModel), vertexBuffer.buffer, verticesCount * sizeof(RenderModel));
    if (newAnimationVertices > 0)
        addToCopylist(sliceFromPodVectorBytes(model.animationVertices), animationVertexBuffer.buffer,
            animatedVerticesCount * sizeof(GltfModel::AnimationVertex));
    flushBarriers(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    endSingleTimeCommands();

    result = models.size();
    models.push_back(ModelData{
        .indiceStart = indicesCount, .indices = newIndices,
        .vertexStart = verticesCount, .vertices = newVertices,
        .animatedVertexStart = animatedVerticesCount, .animatedVectices = newAnimationVertices
    });



    verticesCount += newVertices;
    indicesCount += newIndices;
    animatedVerticesCount += newAnimationVertices;
    return result;
}

void MeshRenderSystem::clear()
{
    for (auto& matrices : modelRenderMatrices)
        matrices.clear();
    for (auto& matrices : animatedModelRenderMatrices)
        matrices.clear();
    for (auto& matrices : boneAnimatedModelRenderMatrices)
        matrices.clear();
}

bool MeshRenderSystem::addModelToRender(uint32_t modelIndex, const Matrix& renderMatrix, const PodVector<Matrix>& boneMatrices)
{
    if (modelIndex >= models.size())
        return false;
    
    if (boneMatrices.size() > 0u)
    {
        if (modelIndex >= boneAnimatedModelRenderMatrices.size())
            boneAnimatedModelRenderMatrices.resize(modelIndex + 1);
        if (modelIndex >= animatedModelRenderMatrices.size())
            animatedModelRenderMatrices.resize(modelIndex + 1);

        boneAnimatedModelRenderMatrices[modelIndex].pushBack(boneMatrices);
        animatedModelRenderMatrices[modelIndex].pushBack(renderMatrix);
    }
    else
    {
        if (modelIndex >= modelRenderMatrices.size())
            modelRenderMatrices.resize(modelIndex + 1);

        modelRenderMatrices[modelIndex].push_back(renderMatrix);
    }

    return true;
}

bool MeshRenderSystem::prepareToRender()
{
    {
        PodVector<Matrix> allModelRenderMatrices;
        for (const auto& vec : animatedModelRenderMatrices)
            allModelRenderMatrices.pushBack(vec);

        for (const auto& vec : modelRenderMatrices)
            allModelRenderMatrices.pushBack(vec);

        if(allModelRenderMatrices.size() > 0)
            addToCopylist(sliceFromPodVectorBytes(allModelRenderMatrices), modelRenderMatricesBuffer);
    }
    {
        PodVector<Matrix> allModelBoneRenderMatrices;
        for (const auto& vec : boneAnimatedModelRenderMatrices)
            allModelBoneRenderMatrices.pushBack(vec);

        if (allModelBoneRenderMatrices.size() > 0)
            addToCopylist(sliceFromPodVectorBytes(allModelBoneRenderMatrices), modelBoneRenderMatricesBuffer);
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
            bool animationRender = (passIndex & 2) == 0;
            uint32_t instances = animationRender ? animatedModelRenderMatrices[modelIndex].size() : modelRenderMatrices[modelIndex].size();
            if (instances)
            {
                const ModelData& modelData = models[modelIndex];
                vkCmdBindIndexBuffer(vulk->commandBuffer, indexDataBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(vulk->commandBuffer, modelData.indices, instances,
                    modelData.indiceStart, modelData.vertexStart, instanceStartIndex);
                instanceStartIndex += instances;
            }
        }
        endDebugRegion();
    }
}

void MeshRenderSystem::render(const Image& renderColorTarget, 
    const Image& normalMapColorImage, const Image& renderDepthTarget)
{
    beginDebugRegion("Mesh rendering colors", Vec4(1.0f, 0.0f, 0.0f, 1.0f));

    static constexpr VkClearValue colorClear = { .color{0.0f, 0.5f, 1.0f, 1.0f} };
    static constexpr VkClearValue normlClear = { .color{0.0f, 0.0f, 0.0f, 0.0f} };
    static constexpr VkClearValue depthClear = { .depthStencil = { 1.0f, 0 } };
    beginRendering({
        RenderImage{.image = &renderColorTarget, .clearValue = colorClear },
        RenderImage{.image = &normalMapColorImage, .clearValue = normlClear }, },
        {.image = &renderDepthTarget, .clearValue = depthClear });
    render(false);
    vkCmdEndRendering(vulk->commandBuffer);
    
    endDebugRegion();
}


void MeshRenderSystem::renderShadows(const Image& shadowDepthTarget)
{
    static constexpr VkClearValue depthClear = { .depthStencil = { 1.0f, 0 } };
    beginDebugRegion("Mesh rendering depth only", Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    
    beginRendering({}, { .image = &shadowDepthTarget, .clearValue = depthClear });
    render(true);
    vkCmdEndRendering(vulk->commandBuffer);
    
    endDebugRegion();
}