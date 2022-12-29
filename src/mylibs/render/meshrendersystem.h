#pragma once

#include <model/gltf.h>

#include <myvulkan/shader.h>
#include <myvulkan/vulkanresources.h>

#include <render/meshrendertargets.h>
#include <scene/gameentity.h>

// Very unoptimized stuff... no culling or anything
class MeshRenderSystem
{
public:
    static bool init();
    static void deinit();
    static bool addModel(const GltfModel& renderModel, EntityType entityType);


    static void clear();

    static bool addModelToRender(uint32_t modelIndex,
        const Mat3x4 &renderMatrix, const Mat3x4 &renderNormalMatrix,
        const PodVector<Mat3x4>& boneAndBoneNormalMatrices);

    static bool prepareToRender();

    static void render(const MeshRenderTargets &meshRenderTargets);
    static void renderShadows(const MeshRenderTargets &meshRenderTargets);

    static void setRenderTargets(const MeshRenderTargets &meshRenderTargets);
};