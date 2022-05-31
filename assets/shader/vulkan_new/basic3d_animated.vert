#version 450 core

#define MATRIX_ORDER row_major
//#define MATRIX_ORDER column_major


layout (binding = 0) uniform ConstantDataStructBlock
{
    vec2 windowSize;
    vec2 padding;
};

layout (binding = 1, MATRIX_ORDER) uniform FrameDataBlock
{
    mat4 cameraMatrix;
    mat4 viewProjMat;
    mat4 mvp;
    mat4 matrix_padding;
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

struct VData
{
    vec4 pos;
    vec4 nor;
    vec4 color;
};

struct AnimationVData
{
    vec4 weights;
    uvec4 boneIndices;
};

layout (std430, binding = 5) restrict readonly buffer vertexData
{
    VData vertexValues[];
};

layout (std430, binding = 6) restrict readonly buffer animationVertexData
{
    AnimationVData animationVertexValues[];
};

layout (location = 0) out flat vec4 colOut;
layout (location = 1) out vec3 normalDirOut;

void main()
{
    uint instanceIndex = gl_InstanceIndex;
    VData data = vertexValues[gl_VertexIndex];
    AnimationVData animData = animationVertexValues[gl_VertexIndex];
    vec4 pos = vec4(0.0f);

    #if 0 
    mat4 boneTransform = animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.x] * animData.weights.x;
    boneTransform += animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.y] * animData.weights.y;
    boneTransform += animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.z] * animData.weights.z;
    boneTransform += animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.w] * animData.weights.w;
    pos = boneTransform * vec4(pos.xyz, 1.0f);
    #else
    pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.x] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.x;
    pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.y] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.y;
    pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.z] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.z;
    pos += (animationBoneMatrices[instanceIndex * 256 + animData.boneIndices.w] * vec4(data.pos.xyz, 1.0f) ) * animData.weights.w;

    #endif
    mat4 finalMat = mvp * enityModelMatrices[instanceIndex];
    gl_Position = finalMat * vec4(pos.xyz, 1.0f);
    vec3 normalDir =  (enityModelMatrices[instanceIndex] * vec4(pos.xyz, 0.0f)).xyz;
    vec3 sunDir = vec3(0.5f, -1.0f, 0.5f);
    colOut = data.color * 0.75f + 0.25f * max(0.0f, -dot(normalDir, sunDir));

    normalDirOut = normalDir;
}