#version 450

//#extension GL_GOOGLE_include_directive : require

#include "shader_common_vulkan.glsl"
#include "common_buffers.glsl"

#define USE_IA 0


struct MeshDraw
{
	float2 offset;
	float2 scale;
};

#if USE_IA
	// fixed vertex
	layout(location = 0) in vec3 position;
	layout(location = 1) in vec3 normal;
	layout(location = 2) in vec2 uv;
#endif

layout(location = 0) out vec4 color;

/*
layout(binding = 2) readonly buffer indices
{
	uint inds[];
};
*/

layout(binding = 8) readonly buffer vertis2
{
	Vertex vertices2[];
};

#if 0
	#define VERTICESARRAY vertices2
#else
	#define VERTICESARRAY vertices
#endif


layout(push_constant) readonly uniform block
{
	MeshDraw meshDraw;
};


//86832
void main()
{
	uint ind = gl_VertexIndex;
	//uint ind = inds[gl_VertexIndex];
	//uint ind = inds[p];
	uint instanceId = gl_InstanceIndex + (uint(ind >> 20u)); // & (1024u - 1u));

	ind = ind & ((1u << 20u) - 1u);
	#if !USE_IA

		vec3 position = VERTICESARRAY[ind].p1.xyz;
	#endif

	float4 posi = float4(position, 1.0f);
	float3 transposedPos;
	// custom matrix multiply with float4 x 3
	transposedPos.x = dot(posi, matrices[instanceId * 3 + 0]);
	transposedPos.y = dot(posi, matrices[instanceId * 3 + 1]);
	transposedPos.z = dot(posi, matrices[instanceId * 3 + 2]);
	gl_Position = proj * float4(transposedPos, 1.0f);

	float3 pos = position * float3(meshDraw.scale, 0.1f);
	pos.xy += meshDraw.offset * 2.0f - 1.0;
	pos.z *= 0.5f;
	pos.z = 0.5f - pos.z;
	//gl_Position = float4(pos, 1.0f);

	#if USE_IA
		float3 norm = normal;
	#else
		float3 norm = float3(VERTICESARRAY[ind].p1.w, VERTICESARRAY[ind].p2.xy);
	#endif

	uint p = (gl_VertexIndex >> 4);
	// Some random "color visualiser thingy"
	float3 f = normalize( float3(p * 1231249358, p * 907596523, p * 1959132374));

	//color = float4(norm * 0.5f + vec3(0.5f), 1.0f);
	color = float4(f, 1.0f);
}


/*
struct Vertex
{
	vec3 pos;
	vec3 normal;
	vec2 texCoord;
};

layout(binding = 0) buffer Vertices
{
	Vertex vertices[];
};

layout(location = 0) out vec4 color;

void main()
{
	Vertex v = vertices[gl_VertexIndex];
	gl_Position = vec4(v.pos, 1.0f);
	color = vec4(v.normal * 0.5f + vec3(0.5f), 1.0f);
}
*/