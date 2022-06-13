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

// Could be packed better
struct AnimatedVData
{
    VData data;
    // packed to uint16_t x 4
    uvec2 weights;
    uint boneIndices;
    uint tmp;
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

layout (std430, binding = 6) restrict readonly buffer animationVertexData
{
    AnimatedVData animationVertexValues[];
};

layout (std430, binding = 7) restrict readonly buffer boneStartIndexData
{
    uint boneStartIndices[];
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
    #if USE_ANIMATION
        AnimatedVData animData = animationVertexValues[gl_VertexIndex];
        VData data = animData.data;
    #else
        VData data = vertexValues[gl_VertexIndex];
    #endif
    vec3 dataNormal = vec3(0.0f);

    dataNormal.x = float(uint(data.normalXYZAttributes.x & 0xffffu)) / 65535.0f * 2.0f - 1.0f;
    dataNormal.y = float(uint(data.normalXYZAttributes.x >> 16u)) / 65535.0f * 2.0f - 1.0f;
    dataNormal.z = float(uint(data.normalXYZAttributes.y & 0xffffu)) / 65535.0f * 2.0f - 1.0f;

    #if USE_ANIMATION
        vec4 pos = vec4(0.0f);
        vec4 weights = vec4(
            (animData.weights.x & 0xffffu),
            (animData.weights.x >> 16u),
            (animData.weights.y & 0xffffu),
            (animData.weights.y >> 16u)) / 65535.0f;
        uvec4 boneIndices = uvec4(
            (animData.boneIndices >> 0 ) & 255,
            (animData.boneIndices >> 8 ) & 255,
            (animData.boneIndices >> 16 ) & 255,
            (animData.boneIndices >> 24 ) & 255 );

        pos += (animationBoneMatrices[boneStartIndices[instanceIndex] + boneIndices.x] * vec4(data.pos.xyz, 1.0f) ) * weights.x;
        pos += (animationBoneMatrices[boneStartIndices[instanceIndex] + boneIndices.y] * vec4(data.pos.xyz, 1.0f) ) * weights.y;
        pos += (animationBoneMatrices[boneStartIndices[instanceIndex] + boneIndices.z] * vec4(data.pos.xyz, 1.0f) ) * weights.z;
        pos += (animationBoneMatrices[boneStartIndices[instanceIndex] + boneIndices.w] * vec4(data.pos.xyz, 1.0f) ) * weights.w;

        #if DEPTH_ONLY
        #else
            vec4 nor = vec4(0.0f);

            nor += (animationBoneMatrices[boneStartIndices[instanceIndex] + boneIndices.x] * vec4(dataNormal.xyz, 0.0f) ) * weights.x;
            nor += (animationBoneMatrices[boneStartIndices[instanceIndex] + boneIndices.y] * vec4(dataNormal.xyz, 0.0f) ) * weights.y;
            nor += (animationBoneMatrices[boneStartIndices[instanceIndex] + boneIndices.z] * vec4(dataNormal.xyz, 0.0f) ) * weights.z;
            nor += (animationBoneMatrices[boneStartIndices[instanceIndex] + boneIndices.w] * vec4(dataNormal.xyz, 0.0f) ) * weights.w;
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