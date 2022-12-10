#pragma once

#include <container/stackstring.h>
#include <container/podvectorsbase.h>
#include <container/vectorsbase.h>
#include <math/bounds.h>
#include <math/matrix.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <model/animation.h>

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
        u32 boneIndices[4];
    };

    struct AnimPos
    {
        Vec3 value;
        float timeStamp = 0.0f;
    };
    struct AnimScale
    {
        Vec3 value;
        float timeStamp = 0.0f;
    };
    // having 5 float doesnt seem to pack nicely? Maybe time stamps should be on separate arrays?
    struct AnimRot
    {
        Quat value;
        float timeStamp = 0.0f;
    };

    struct AnimationIndexData
    {
        // which is the start index for example animation 1, animation samples
        u32 posStartIndex;
        u32 posIndexCount;
        u32 rotStartIndex;
        u32 rotIndexCount;
        u32 scaleStartIndex;
        u32 scaleIndexCount;

        // For a node of a bone, the children indices
        u32 childStartIndex;
        u32 childIndexCount;
    };

    struct ModelMesh
    {
        // bounding boxes
        Bounds bounds;

        PodVector<Vertex> vertices;
        PodVector<Vec4> vertexColors;
        PodVector<Vec2> vertexUvs;
        PodVector<AnimationVertex> animationVertices;
        PodVector<u32> indices;
        SmallStackString meshName;
    };

    Vector<ModelMesh> modelMeshes;

    PodVector<Mat3x4> inverseMatrices;
    PodVector<Mat3x4> inverseNormalMatrices;

    // These are indices to animationPosData, animationRotData and animationScaleData, and childrenJointIndices.
    Vector<PodVector<AnimationIndexData>> animationIndices;

    // pos, rot scale animation data. Each animation data is just set after each other, to get indices, must use animationIndices
    PodVector<AnimPos> animationPosData;
    PodVector<AnimRot> animationRotData;
    PodVector<AnimScale> animationScaleData;
    PodVector<u32> childrenJointIndices;

    PodVector<float> animStartTimes;
    PodVector<float> animEndTimes;
    PodVector<SmallStackString> animNames;
};

bool evaluateAnimation(const GltfModel &model, u32 animationIndex, float time,
    PodVector<Mat3x4> &outMatrices);

bool evaluateAnimation(const GltfModel &model, AnimationState &animationState,
    PodVector<Mat3x4> &outMatrices);



bool readGLTF(const char *filename, GltfModel &outModel);


