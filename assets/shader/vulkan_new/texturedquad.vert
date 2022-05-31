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
    vec2 vpos;
    uint vSizes;
    uint vColor;

    vec2 vUvStart;
    vec2 vUvSize;
};

layout (binding = 1) uniform ValueBlock
{
    VData values[2048];
};


layout (location = 0) out vec4 colOut;
layout (location = 1) out vec2 uvOut;
void main()
{
    int quadId = gl_VertexIndex / 4;
    int vertId = gl_VertexIndex % 4;

    vec2 p = vec2(0.0f);
    p.x = (vertId + 1) % 4 < 2 ? 0.0f : 1.0f;
    p.y = vertId < 2 ? 0.0f : 1.0f;

    uvOut = p * values[quadId].vUvSize;
    uvOut += values[quadId].vUvStart;

    vec2 vSize = vec2(float(values[quadId].vSizes & 0xffffu),
        float((values[quadId].vSizes >> 16) & 0xffffu));
    p *= vSize;
    p += values[quadId].vpos;
    p.y = windowSize.y - p.y;
    p /= windowSize * 0.5f;
    p -= 1.0f;
    //p.y = 1.0f - p.y;
    gl_Position = vec4(p.xy, 0.5, 1.0);
    vec4 c = vec4(0, 0, 0, 0);
    c.r = float((values[quadId].vColor >> 0u) & 255u) / 255.0f;
    c.g = float((values[quadId].vColor >> 8u) & 255u) / 255.0f;
    c.b = float((values[quadId].vColor >> 16u) & 255u) / 255.0f;
    c.a = float((values[quadId].vColor >> 24u) & 255u) / 255.0f;
    colOut = c;
}