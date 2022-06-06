#version 450 core

#define DEPTH_ONLY ((SPECIAL_PERMUTATION & 1) == 1)
#define USE_ANIMATION ((SPECIAL_PERMUTATION & 2) == 0)
#define USE_LINE (SPECIAL_PERMUTATION == 5)

#include "common.h"


// Could be packed better
struct VData
{
    vec3 pos;
    uint color;

    vec2 uv;

    // 16 bit usigned x, y, z then Attributes
    // 1, use vertexcolor, 2, use uvs
    uvec2 normalXYZAttributes;
};

layout (binding = 2, MATRIX_ORDER) restrict readonly buffer EnityModelMatrices
{
    mat4 enityModelMatrices[];
};


layout (binding = 3, MATRIX_ORDER) restrict readonly buffer AnimationBoneMatrices
{
    mat4 animationBoneMatrices[];
};

layout (binding = 4, MATRIX_ORDER) restrict readonly buffer AnimationDataNormals
{
    mat3x3 normalInverses[];
};

layout (std430, binding = 5) restrict readonly buffer vertex_data
{
    VData vertexValues[];
};

struct AnimationVData
{
    vec4 weights;
    uvec4 boneIndices;
};

layout (std430, binding = 6) restrict readonly buffer animationVertexData
{
    AnimationVData animationVertexValues[];
};

#if DEPTH_ONLY
#else
    layout (location = 0) out vec4 colOut;
    layout (location = 1) out vec3 normalDirOut;
    layout (location = 2) out flat uint attributesOut;
    layout (location = 3) out flat vec2 uvOut;
#endif

void main()
{
    uint instanceIndex = gl_InstanceIndex;
    VData data = vertexValues[gl_VertexIndex];

    vec3 dataNormal = vec3(0.0f);

    dataNormal.x = float(uint(data.normalXYZAttributes.x & 0xffffu)) / 65535.0f * 2.0f - 1.0f;
    dataNormal.y = float(uint(data.normalXYZAttributes.x >> 16u)) / 65535.0f * 2.0f - 1.0f;
    dataNormal.z = float(uint(data.normalXYZAttributes.y & 0xffffu)) / 65535.0f * 2.0f - 1.0f;

    #if USE_ANIMATION
        AnimationVData animData = animationVertexValues[gl_VertexIndex];
        vec4 pos = vec4(0.0f);
        pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.x] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.x;
        pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.y] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.y;
        pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.z] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.z;
        pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.w] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.w;

        #if DEPTH_ONLY
        #else
            vec4 nor = vec4(0.0f);

            nor += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.x] * vec4(dataNormal.xyz, 0.0f) ) * animData.weights.x;
            nor += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.y] * vec4(dataNormal.xyz, 0.0f) ) * animData.weights.y;
            nor += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.z] * vec4(dataNormal.xyz, 0.0f) ) * animData.weights.z;
            nor += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.w] * vec4(dataNormal.xyz, 0.0f) ) * animData.weights.w;
        #endif
    #else
        vec4 pos = vec4(data.pos.xyz, 1.0f);
        vec4 nor = vec4(dataNormal.xyz, 0.0f);
    #endif

    #if DEPTH_ONLY
        mat4 finalMat = sunMatrix * enityModelMatrices[instanceIndex];
    #else
        mat4 finalMat = mvp * enityModelMatrices[instanceIndex];
    #endif

    gl_Position = finalMat * vec4(pos.xyz, 1.0f);
    #if DEPTH_ONLY
    #else
        colOut = vec4(
            float((data.color >> 0) & 255),
            float((data.color >> 8) & 255),
            float((data.color >> 16) & 255),
            float((data.color >> 24) & 255)) / 255.0f;

        vec3 normalDir =  normalize(enityModelMatrices[instanceIndex] * vec4(normalize(nor.xyz), 0.0f)).xyz;
        normalDirOut = normalDir.xyz;
        attributesOut = (data.normalXYZAttributes.y >> 16);
        uvOut = data.uv;
    #endif
}