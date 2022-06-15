#version 450 core

#define PERMUTATIONS 4
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
    mat4x3 enityModelMatrices[];
};


layout (binding = 3, MATRIX_ORDER) restrict readonly buffer AnimationBoneMatrices
{
    mat4x3 animationBoneMatrices[];
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

        uint boneStartIndex = boneStartIndices[instanceIndex];
        mat4x3 boneMat = animationBoneMatrices[boneStartIndex + boneIndices.x] * weights.x;
        boneMat += animationBoneMatrices[boneStartIndex + boneIndices.y] * weights.y;
        boneMat += animationBoneMatrices[boneStartIndex + boneIndices.z] * weights.z;
        boneMat += animationBoneMatrices[boneStartIndex + boneIndices.w] * weights.w;

        mat4 boneMat4;
        boneMat4[0] = vec4(boneMat[0], 0.0f);
        boneMat4[1] = vec4(boneMat[1], 0.0f);
        boneMat4[2] = vec4(boneMat[2], 0.0f);
        boneMat4[3] = vec4(boneMat[3], 1.0f);

        pos = boneMat4 *  vec4(data.pos.xyz, 1.0f);

        #if DEPTH_ONLY
        #else
            vec4 nor = normalize(vec4(1.0f));

            uint boneNormalStartIndex = boneStartIndices[instanceIndex];
            mat4x3 boneNormalMat4x3 = animationBoneMatrices[boneNormalStartIndex + boneIndices.x] * weights.x;
            boneNormalMat4x3 += animationBoneMatrices[boneNormalStartIndex + boneIndices.y] * weights.y;
            boneNormalMat4x3 += animationBoneMatrices[boneNormalStartIndex + boneIndices.z] * weights.z;
            boneNormalMat4x3 += animationBoneMatrices[boneNormalStartIndex + boneIndices.w] * weights.w;

            mat4 boneNormalMat4;
            boneNormalMat4[0] = vec4(boneNormalMat4x3[0], 0.0f);
            boneNormalMat4[1] = vec4(boneNormalMat4x3[1], 0.0f);
            boneNormalMat4[2] = vec4(boneNormalMat4x3[2], 0.0f);
            boneNormalMat4[3] = vec4(boneNormalMat4x3[3], 1.0f);

            nor = boneNormalMat4 *  vec4(dataNormal.xyz, 0.0f);
        #endif
    #else
        vec4 pos = vec4(data.pos.xyz, 1.0f);
        vec4 nor = vec4(dataNormal.xyz, 0.0f);
    #endif
    mat4x3 entityMatrix4x3 = enityModelMatrices[instanceIndex];
    mat4 entityMatrix;
    entityMatrix[0] = vec4(entityMatrix4x3[0], 0.0f);
    entityMatrix[1] = vec4(entityMatrix4x3[1], 0.0f);
    entityMatrix[2] = vec4(entityMatrix4x3[2], 0.0f);
    entityMatrix[3] = vec4(entityMatrix4x3[3], 1.0f);
    #if DEPTH_ONLY
        mat4 finalMat = sunMatrix * entityMatrix;
    #else
        mat4 finalMat = mvp * entityMatrix;
    #endif

    gl_Position = finalMat * pos;
    #if DEPTH_ONLY
    #else
        colOut = vec4(
            float((data.color >> 0) & 255),
            float((data.color >> 8) & 255),
            float((data.color >> 16) & 255),
            float((data.color >> 24) & 255)) / 255.0f;

        vec3 normalDir =  normalize(entityMatrix * vec4(normalize(nor.xyz), 0.0f)).xyz;
        normalDirOut = normalDir.xyz;
        attributesOut = (data.normalXYZAttributes.y >> 16);
        uvOut = data.uv;
    #endif
}