#pragma once

#include <container/podvector.h>
#include <container/vector.h>
#include <math/matrix.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <string_view>

struct GltfModel
{
    struct Vertex
    {
        Vec3 pos;
        float padding1;
        Vec3 norm;
        float padding2;
    };
    struct AnimationVertex
    {
        Vec4 weights;
        uint32_t boneIndices[4];
    };

    struct BoneAnimationPosOrScale
    {
        Vec3 value;
        float timeStamp = 0.0f;
    };
    struct BoneAnimationRot
    {
        Quat value;
        float timeStamp = 0.0f;
    };
    struct Bone
    {
        PodVector<uint32_t> childrenIndices;
    };

    PodVector<Vertex> vertices;
    PodVector<Vec4> vertexColors;
    PodVector<Vec2> vertexUvs;
    PodVector<AnimationVertex> animationVertices;
    PodVector<uint32_t> indices;
    PodVector<Matrix> inverseMatrices;

    // animation index, boneindex?
    Vector<Vector<PodVector<BoneAnimationPosOrScale>>> animationPosData;
    Vector<Vector<PodVector<BoneAnimationRot>>> animationRotData;
    Vector<Vector<PodVector<BoneAnimationPosOrScale>>> animationScaleData;
    Vector<Bone> bones;
    uint32_t boneCount = 0u;

    PodVector<float> animStartTimes;
    PodVector<float> animEndTimes;
};

bool evaluateAnimation(const GltfModel &model, uint32_t animationIndex, float time,
    PodVector<Matrix> &outMatrices);


bool readGLTF(std::string_view filename, GltfModel &outModel);
