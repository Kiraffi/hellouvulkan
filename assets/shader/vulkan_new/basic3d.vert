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

    vec3 sunDir = vec3(0.5f, -1.0f, 0.5f);
    colOut = data.color * 0.75f + 0.25f * max(0.0, -dot(normalDir, sunDir));
    normalDirOut = normalDir;
}