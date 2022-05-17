#pragma once

#include <vector>
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

    struct AnimationPoint
    {
        Vec4 rot;
        Vec3 pos;
        Vec3 scale;
        uint32_t boneIndex = ~0u;
        float timePoint = 0.0f;
    };

    std::vector<Vertex> vertices;
    std::vector<AnimationVertex> animationVertices;
    std::vector<uint32_t> indices;


    std::vector<std::vector<AnimationPoint>> animationData;
};

bool readGLTF(const char *filename, RenderModel &outModel);
