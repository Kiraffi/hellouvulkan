#define COMPUTE_LOCAL_INDEX_SIZE 32


struct Vertex
{
	float4 p1;
	float4 p2;
	//vec3 position;
	//vec3 normal;
	//vec2 texCoord;
};


struct VertexCompute 
{
	float4 p1;
	float4 p2;
};

struct IndirectValues
{
	uint myAtomicValue;
	uint instanceCount;
	uint indexOffset;
	uint vertexOffset;
	uint instanceOffset;

	uint padding0;
	uint padding1;
	uint vertexAtomicStartIndex;
};

struct MeshTypeData
{
	uint vertexStartIndex;
	uint vertexAmount;
	uint indiceStartIndex;
	uint indiceAmount;
	uint meshletStartIndex;
	uint meshletAmount;
	uint padding1;
	uint padding2;
};
 

struct MeshInstanceData
{
	uint instanceModelType;
	uint meshletGroupIndex;
	uint instanceIndex;
	uint meshletWriteCount;
};

struct Meshlet
{
	float3 normal;
	float normalAngleMax;

	float3 pos;
	float radius;
};


// either stdlayout430 or 140 forces
//forces 16 bits, so basically its actually uvec4 layout(std140, binding = 0) buffer buf
// accepts non 4byte alignment per index.

layout(binding = 0) readonly uniform uniformb
{
	float4 matrices[1200 * 3];
	mat4 proj;
	mat4 camMat;
	float4 cameraForwardDir; // padded
	float4 cameraPosition; // padded
	float4 screenSize; // padded
	float4 cameraAspectRatio;

	float4 frustumPlanes[6];
	uint4 drawCountValues;
	float4 padding2;
	uint4 alphabets[256 / 4];
};


layout(binding = 1) readonly buffer vertis
{
	Vertex vertices[];
};




layout(binding = 5) readonly buffer meshletBuffer
{
	Meshlet meshlets[];
};

layout(binding = 6) readonly buffer instanceDataBuffer
{
	MeshTypeData instanceDatas[2];

	uint4 instanceIndexes[];
};
