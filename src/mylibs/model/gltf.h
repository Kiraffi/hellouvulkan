#pragma once

#include <container/podvector.h>
#include <container/vector.h>
#include <math/matrix.h>
#include <math/quaternion.h>
#include <math/vector3.h>

struct RenderModel
{
    struct Vertex
    {
        Vec4 pos;
        Vec4 norm;
        Vec4 color;
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
    PodVector<AnimationVertex> animationVertices;
    PodVector<uint32_t> indices;
    PodVector<Matrix> inverseMatrices;

    Vector<PodVector<BoneAnimationPosOrScale>> animationPosData;
    Vector<PodVector<BoneAnimationRot>> animationRotData;
    Vector<PodVector<BoneAnimationPosOrScale>> animationScaleData;
    Vector<Bone> bones;
    uint32_t boneCount = 0u;

    float animStartTime = 0.0f;
    float animEndTime = 0.0f;
};

bool evaluateAnimation(const RenderModel &model, uint32_t animIndex, float time,
    PodVector<Matrix> &outMatrices);


bool readGLTF(const char *filename, RenderModel &outModel);
