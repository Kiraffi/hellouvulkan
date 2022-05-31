#version 450 core

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

layout (std430, binding = 5) restrict readonly buffer vertex_data
{
    VData vertexValues[];
};

layout (location = 0) out flat vec4 colOut;
layout (location = 1) out vec3 normalDirOut;

void main()
{
    uint instanceIndex = gl_InstanceIndex;
    VData data = vertexValues[gl_VertexIndex];
    mat4 finalMat = mvp * enityModelMatrices[instanceIndex];
    gl_Position = finalMat * vec4(data.pos.xyz, 1.0f);
    vec3 normalDir =  (enityModelMatrices[instanceIndex] * vec4(data.nor.xyz, 0.0f)).xyz;

    colOut = data.color;
    normalDirOut = normalDir;
}