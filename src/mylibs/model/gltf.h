#pragma once

#include <container/podvector.h>
#include <math/vector3.h>

struct RenderModel
{
    struct Vertex
    {
        Vec4 pos;
        Vec4 norm;
        Vec4 color;
    };
    struct AnimationVertexData
    {
        Vec4 weights;
        uint32_t indices[4];
    };

    PodVector<Vertex> vertices;
    PodVector<AnimationVertexData> animationVertices;
    PodVector<uint32_t> indices;
};

struct ModelBone
{

};

bool readGLTF(const char *filename, RenderModel &outModel);
