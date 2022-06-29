#pragma once

#include <container/stackstring.h>
#include <container/podvector.h>
#include <container/vector.h>
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
        uint32_t boneIndices[4];
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
        uint32_t posStartIndex;
        uint32_t posIndexCount;
        uint32_t rotStartIndex;
        uint32_t rotIndexCount;
        uint32_t scaleStartIndex;
        uint32_t scaleIndexCount;

        // For a node of a bone, the children indices
        uint32_t childStartIndex;
        uint32_t childIndexCount;
    };

    struct ModelMesh
    {
        // bounding boxes
        Bounds bounds;

        PodVector<Vertex> vertices;
        PodVector<Vec4> vertexColors;
        PodVector<Vec2> vertexUvs;
        PodVector<AnimationVertex> animationVertices;
        PodVector<uint32_t> indices;
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
    PodVector<uint32_t> childrenJointIndices;

    PodVector<float> animStartTimes;
    PodVector<float> animEndTimes;
    PodVector<SmallStackString> animNames;
};

bool evaluateAnimation(const GltfModel &model, uint32_t animationIndex, float time,
    PodVector<Mat3x4> &outMatrices);

bool evaluateAnimation(const GltfModel &model, AnimationState &animationState,
    PodVector<Mat3x4> &outMatrices);



bool readGLTF(const char *filename, GltfModel &outModel);


