#pragma once

#include <string_view>

#include <model/gltf.h>

#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

#include <render/meshrendertargets.h>
#include <scene/gameentity.h>

// Very unoptimized stuff... no culling or anything
class MeshRenderSystem
{
public:
    ~MeshRenderSystem();

    bool init();
    bool addModel(const GltfModel& renderModel, EntityType entityType);


    void clear();

    bool addModelToRender(uint32_t modelIndex, const Mat3x4 &renderMatrix, const Mat3x4 &renderNormalMatrix, 
        const PodVector<Mat3x4>& boneAndBoneNormalMatrices);

    bool prepareToRender();

    void render(const MeshRenderTargets &meshRenderTargets);
    void renderShadows(const MeshRenderTargets &meshRenderTargets);

    void setRenderTargets(const MeshRenderTargets &meshRenderTargets);
private:

    void render(bool isShadowOnly);

    // maybe there should be something that indicates its animated model or not?
    struct ModelData
    {
        uint32_t indiceStart = 0u;
        uint32_t indices = 0u;
        uint32_t vertexStart = 0u;
        uint32_t vertices = 0u;
    };

    Buffer vertexBuffer;
    Buffer animationVertexBuffer;
    Buffer indexDataBuffer;

    Buffer modelRenderMatricesBuffer[VulkanGlobal::FramesInFlight];
    Buffer modelBoneRenderMatricesBuffer[VulkanGlobal::FramesInFlight];
    Buffer modelRenderBoneStartIndexBuffer[VulkanGlobal::FramesInFlight];

    Image paletteImage;

    Pipeline meshRenderGraphicsPipeline[4];

    PodVector<ModelData> models;

    // these 3 probably should belong somewhere else, since they depend on scenedata, if wanting to have render to texture...
    // maybe MeshRenderScene
    Vector<PodVector< uint32_t >> modelRenderBoneStartIndices;
    Vector<PodVector< Mat3x4 >> modelRenderMatrices;
    Vector<PodVector< Mat3x4 >> animatedModelRenderMatrices;
    PodVector< Mat3x4 > boneAnimatedModelRenderMatrices;

    uint32_t indicesCount = 0u;
    uint32_t verticesCount = 0u;
    uint32_t animatedVerticesCount = 0u;
};