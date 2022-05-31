#pragma once

#include <string_view>

#include <model/gltf.h>
#include <myvulkan/vulkanresources.h>


// Very unoptimized stuff... no culling or anything
class MeshRenderSystem
{
public:
    ~MeshRenderSystem();

    bool init(UniformBufferHandle uniformDataHandle);
    uint32_t addModel(const RenderModel &renderModel);


    void clear();

    bool addModelToRender(uint32_t modelIndex, const Matrix& renderMatrix, const PodVector<Matrix>& boneMatrices);

    bool prepareToRender();

    void render(const Image& renderColorTarget, const Image& normalMapColorImage, const Image& renderDepthTarget);

private:
    struct ModelData
    {
        uint32_t indiceStart = 0u;
        uint32_t indices = 0u;
        uint32_t vertexStart = 0u;
        uint32_t vertices = 0u;
        uint32_t animatedVertexStart = 0u;
        uint32_t animatedVectices = 0u;
    };

    Buffer vertexBuffer;
    Buffer animationVertexBuffer;
    Buffer indexDataBuffer;

    Buffer modelRenderMatricesBuffer;
    Buffer modelBoneRenderMatricesBuffer;
    Buffer modelRenderNormaMatricesBuffer;

    Pipeline animatedGraphicsPipeline;
    Pipeline nonAnimatedGraphicsPipeline;

    PodVector<ModelData> models;

    // these 3 probably should belong somewhere else, since they depend on scenedata, if wanting to have render to texture...
    // maybe MeshRenderScene
    Vector<PodVector< Matrix >> modelRenderMatrices;
    Vector<PodVector< Matrix >> animatedModelRenderMatrices;
    Vector<PodVector< Matrix >> boneAnimatedModelRenderMatrices;

    uint32_t indicesCount = 0u;
    uint32_t verticesCount = 0u;
    uint32_t animatedVerticesCount = 0u;
};