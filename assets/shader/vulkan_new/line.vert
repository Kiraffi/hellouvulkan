#version 450 core

#include "common.h"


struct VData
{
    vec3 pos;
    uint color;
};

layout (std430, binding = 5) restrict readonly buffer vertex_data
{
    VData vertexValues[];
};

layout (location = 0) out vec4 colOut;

void main()
{
    uint instanceIndex = gl_InstanceIndex;
    VData data = vertexValues[gl_VertexIndex];
    gl_Position = mvp * vec4(data.pos.xyz, 1.0f);

    colOut = vec4(
        float((data.color >> 0) & 255),
        float((data.color >> 8) & 255),
        float((data.color >> 16) & 255),
        float((data.color >> 24) & 255)) / 255.0f;
}