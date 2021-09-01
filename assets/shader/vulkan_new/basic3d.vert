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
	vec4 nor;
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
	mat4 finalMat = mvp * matrix_padding;
	gl_Position = finalMat * vec4(data.pos.xyz, 1.0f);
	vec3 norm =  (matrix_padding * vec4(data.nor.xyz, 0.0f)).xyz;
	vec3 sunDir = vec3(0.5f, -1.0f, 0.5f);
	colOut = data.color * 0.75f + 0.25f * (-dot(norm, sunDir));
}