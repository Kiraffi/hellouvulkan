#version 450 core

layout (binding = 0) uniform ConstantDataStructBlock
{
	vec2 windowSize;
	vec2 padding;

};

layout (binding = 1) uniform FrameDataBlock
{
	mat4 cameraMatrix;
	mat4 viewProjMat;
	mat4 mvp;
	mat4 matrix_padding;
};




struct VData
{
	vec4 pos;
	vec4 norm;
	vec4 color;
};

layout (std430, binding=2) restrict readonly buffer vertex_data
{
	VData vertexValues[];
};

layout (location = 0) out flat vec4 colOut;

void main()
{
	VData data = vertexValues[gl_VertexIndex];
	//gl_Position = mvp * vec4(data.pos.xyz, 1.0f);
	vec3 p = data.pos.xyz;
	//p.xy = p.xy * 0.5f;
	//p.z = p.z * 0.25f + 0.5f;
	gl_Position = mvp  * (vec4(p, 1.0f) * matrix_padding);
	colOut = data.color;
}