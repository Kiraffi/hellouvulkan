#version 450 core

#include "common.h"

struct VData
{
    vec2 vpos;
    uint vSizes;
    uint vColor;
};

layout (std430, binding=1) restrict readonly buffer shader_data
{
    VData values[];
};


layout (location = 0) out vec4 colOut;
void main()
{
    int quadId = gl_VertexIndex / 4;
    int vertId = gl_VertexIndex % 4;

    vec2 p = vec2(-0.5f, -0.5f);
    p.x = (vertId + 1) % 4 < 2 ? -0.5f : 0.5f;
    p.y = vertId < 2 ? -0.5f : 0.5f;

    p += 0.5f;

    vec2 vSize = vec2(float(values[quadId].vSizes & 0xffffu),
        float((values[quadId].vSizes >> 16) & 0xffffu));
    p *= vSize;
    p += values[quadId].vpos;
    p.y = windowSize.y - p.y;
    p /= windowSize * 0.5f;
    p -= 1.0f;
    //p.y = -p.y;
    gl_Position = vec4(p.xy, 0.5, 1.0);
    vec4 c = vec4(0, 0, 0, 0);
    c.r = float((values[quadId].vColor >> 0u) & 255u) / 255.0f;
    c.g = float((values[quadId].vColor >> 8u) & 255u) / 255.0f;
    c.b = float((values[quadId].vColor >> 16u) & 255u) / 255.0f;
    c.a = float((values[quadId].vColor >> 24u) & 255u) / 255.0f;
    colOut = c;
}