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

layout (binding = 2, MATRIX_ORDER) uniform AnimationData
{
    mat4 animationVertices[256];
    mat3x3 normalInverses[256];
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

layout (std430, binding=3) restrict readonly buffer vertexData
{
    VData vertexValues[];
};

layout (std430, binding=4) restrict readonly buffer animationVertexData
{
    AnimationVData animationVertexValues[];
};

layout (location = 0) out flat vec4 colOut;

void main()
{
    VData data = vertexValues[gl_VertexIndex];
    AnimationVData animData = animationVertexValues[gl_VertexIndex];
    vec4 pos = vec4(data.pos.xyz, 1.0f);

    mat4 boneTransform = animationVertices[animData.boneIndices.x] * animData.weights.x;
    boneTransform += animationVertices[animData.boneIndices.y] * animData.weights.y;
    boneTransform += animationVertices[animData.boneIndices.z] * animData.weights.z;
    boneTransform += animationVertices[animData.boneIndices.w] * animData.weights.w;
    pos = boneTransform * vec4(pos.xyz, 1.0f);
    mat4 finalMat = mvp * matrix_padding;
    gl_Position = finalMat * vec4(pos.xyz, 1.0f);
    vec3 norm =  (matrix_padding * vec4(pos.xyz, 0.0f)).xyz;
    vec3 sunDir = vec3(0.5f, -1.0f, 0.5f);
    colOut = data.color; // * 0.75f + 0.25f * (-dot(norm, sunDir));
}