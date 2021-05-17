#version 450 core

layout (location = 0) uniform vec2 windowSize;

struct VData
{
	vec2 vpos;
	uint vSizes;
	uint vColor;

	vec2 vUv;
	vec2 tmp;
};

layout (std430, binding=0) buffer shader_data
{
	VData values[];
};


layout (location = 0) out vec4 colOut;
layout (location = 1) out vec2 uvOut;
void main()
{
	int quadId = gl_VertexID / 4;
	int vertId = gl_VertexID % 4;

	vec2 p = vec2(-0.5f, -0.5f);
	p.x = (vertId + 1) % 4 < 2 ? -0.5f : 0.5f;
	p.y = vertId < 2 ? -0.5f : 0.5f;

	uvOut = p + 0.5f;
		
	uvOut.x = (uvOut.x) / (128.0f - 32.0f) + values[quadId].vUv.x;

	vec2 vSize = vec2(float(values[quadId].vSizes & 0xffffu),
		float((values[quadId].vSizes >> 16) & 0xffffu)); 
	p *= vSize;
	p += values[quadId].vpos;
	p /= windowSize * 0.5f;
	p -= 1.0f;
		
	gl_Position = vec4(p.xy, 0.5, 1.0);
	vec4 c = vec4(0, 0, 0, 0);
	c.r = float((values[quadId].vColor >> 0u) & 255u) / 255.0f;
	c.g = float((values[quadId].vColor >> 8u) & 255u) / 255.0f;
	c.b = float((values[quadId].vColor >> 16u) & 255u) / 255.0f;
	c.a = float((values[quadId].vColor >> 24u) & 255u) / 255.0f;
	colOut = c;
}