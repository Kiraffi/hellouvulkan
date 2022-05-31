#version 450 core

#define USE_ANIMATION (SPECIAL_PERMUTATION > 2)

#define MATRIX_ORDER row_major
//#define MATRIX_ORDER column_major

layout (binding = 0, MATRIX_ORDER) uniform ConstantDataStructBlock
{
    mat4 cameraMatrix;
    mat4 viewProjMat;
    mat4 mvp;
    mat4 matrix_padding;

    vec2 windowSize;
    float padding[12];

};

struct VData
{
    vec4 pos;
    vec4 nor;
    vec4 color;
};

layout (binding = 2, MATRIX_ORDER) restrict readonly buffer EnityModelMatrices
{
    mat4 enityModelMatrices[];
};



#if USE_ANIMATION

layout (binding = 3, MATRIX_ORDER) restrict readonly buffer AnimationBoneMatrices
{
    mat4 animationBoneMatrices[];
};

layout (binding = 4, MATRIX_ORDER) restrict readonly buffer AnimationDataNormals
{
    mat3x3 normalInverses[];
};
#endif


layout (std430, binding = 5) restrict readonly buffer vertex_data
{
    VData vertexValues[];
};

#if USE_ANIMATION
struct AnimationVData
{
    vec4 weights;
    uvec4 boneIndices;
};

layout (std430, binding = 6) restrict readonly buffer animationVertexData
{
    AnimationVData animationVertexValues[];
};
#endif

layout (location = 0) out flat vec4 colOut;
layout (location = 1) out vec3 normalDirOut;

void main()
{
    uint instanceIndex = gl_InstanceIndex;
    VData data = vertexValues[gl_VertexIndex];

    #if USE_ANIMATION
        AnimationVData animData = animationVertexValues[gl_VertexIndex];
        vec4 pos = vec4(0.0f);
        pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.x] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.x;
        pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.y] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.y;
        pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.z] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.z;
        pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.w] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.w;


        vec4 nor = vec4(0.0f);
        nor += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.x] * vec4(data.nor.xyz, 1.0f) ) * animData.weights.x;
        nor += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.y] * vec4(data.nor.xyz, 1.0f) ) * animData.weights.y;
        nor += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.z] * vec4(data.nor.xyz, 1.0f) ) * animData.weights.z;
        nor += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.w] * vec4(data.nor.xyz, 1.0f) ) * animData.weights.w;
    #else
        vec4 pos = vec4(data.pos.xyz, 1.0f);
        vec4 nor = vec4(data.nor.xyz, 0.0f);
    #endif


    mat4 finalMat = mvp * enityModelMatrices[instanceIndex];
    gl_Position = finalMat * vec4(pos.xyz, 1.0f);
    vec3 normalDir =  normalize(enityModelMatrices[instanceIndex] * vec4(normalize(nor.xyz), 0.0f)).xyz;

    colOut = data.color;
    normalDirOut = normalDir.xyz;
}