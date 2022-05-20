#pragma once

#include <container/podvector.h>
#include <container/vector.h>
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
        Vec4 value;
        float timeStamp = 0.0f;
    };

    PodVector<Vertex> vertices;
    PodVector<AnimationVertex> animationVertices;
    PodVector<uint32_t> indices;

    Vector<PodVector<BoneAnimationPosOrScale>> animationPosData;
    Vector<PodVector<BoneAnimationRot>> animationRotData;
    Vector<PodVector<BoneAnimationPosOrScale>> animationScaleData;
    uint32_t boneCount = 0u;
};

bool readGLTF(const char *filename, RenderModel &outModel);
