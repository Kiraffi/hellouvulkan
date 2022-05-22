#version 450 core

layout (binding = 0) uniform ConstantDataStructBlock
{
    vec2 windowSize;
    vec2 padding;
};

struct VData
{
    vec2 vpos;
    //uint vModelIndex;
};

struct IData
{
    uint iPos;
    uint iSinCosRotationSize;
    uint iColor;
    uint iModelVertexStartIndex;
};

layout (std430, binding=2) restrict readonly buffer vertex_data
{
    VData vertexValues[];
};

layout (std430, binding=3) restrict readonly buffer instance_data
{
    IData instanceValues[];
};

layout (location = 0) out flat vec4 colOut;

void main()
{

    uint modelInstance = (gl_VertexIndex >> 16);
    int indice = (gl_VertexIndex & 0xffff);

    vec2 p = vertexValues[instanceValues[modelInstance].iModelVertexStartIndex + indice].vpos.xy;

    uint posrotsize = instanceValues[modelInstance].iSinCosRotationSize;
    p *= float((posrotsize & ((1u << 10u) - 1u))) / 1023.0f * 64.0f;
    float sv = float(((posrotsize >> 10u) & ((1u << 10u) - 1))) / 1023.0f * 2.0f - 1.0f;
    float cv = float(((posrotsize >> 20u) & ((1u << 10u) - 1))) / 1023.0f * 2.0f - 1.0f;
    p = vec2(	cv * p.x - sv * p.y,
                sv * p.x + cv * p.y);

    // Offset
    vec2 posOffset = vec2(float(instanceValues[modelInstance].iPos & 0xffffu),
        float(instanceValues[modelInstance].iPos >> 16));
    posOffset /= 65535.0f;
    posOffset *= 32768.0f;
    p += posOffset;



    //p += instanceValues[modelInstance].iPos.xy;
    // Move into NDC
    p /= windowSize * 0.5f;
    p -= 1.0f;

    gl_Position = vec4(p.xy, 0.5, 1.0);
    vec4 c = vec4(0, 0, 0, 0);
    uint iColor = instanceValues[modelInstance].iColor;

    c.r = float((iColor >> 0u) & 255u);
    c.g = float((iColor >> 8u) & 255u);
    c.b = float((iColor >> 16u) & 255u);
    c.a = float((iColor >> 24u) & 255u);
    c /= 255.0f;

    colOut = c;
}