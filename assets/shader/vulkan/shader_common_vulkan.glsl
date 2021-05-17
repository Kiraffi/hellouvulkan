#define float2 vec2
#define float3 vec3
#define float4 vec4

#define int2 ivec2
#define int3 ivec3
#define int4 ivec4

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

bvec4 greater(float4 v, float4 comp)
{
	return bvec4(v.x > comp.x, v.y > comp.y, v.z > comp.z, v.w > comp.w);
}
bvec4 lesser(float4 v, float4 comp)
{
	return bvec4(v.x < comp.x, v.y < comp.y, v.z < comp.z, v.w < comp.w);
}
bvec4 greaterEq(float4 v, float4 comp)
{
	return bvec4(v.x >= comp.x, v.y >= comp.y, v.z >= comp.z, v.w >= comp.w);
}
bvec4 lesserEq(float4 v, float4 comp)
{
	return bvec4(v.x <= comp.x, v.y <= comp.y, v.z <= comp.z, v.w <= comp.w);
}


bvec3 greater(float3 v, float3 comp)
{
	return bvec3(v.x > comp.x, v.y > comp.y, v.z > comp.z);
}
bvec3 lesser(float3 v, float3 comp)
{
	return bvec3(v.x < comp.x, v.y < comp.y, v.z < comp.z);
}
bvec3 greaterEq(float3 v, float3 comp)
{
	return bvec3(v.x >= comp.x, v.y >= comp.y, v.z >= comp.z);
}
bvec3 lesserEq(float3 v, float3 comp)
{
	return bvec3(v.x <= comp.x, v.y <= comp.y, v.z <= comp.z);
}


bvec2 greater(float2 v, float2 comp)
{
	return bvec2(v.x > comp.x, v.y > comp.y);
}
bvec2 lesser(float2 v, float2 comp)
{
	return bvec2(v.x < comp.x, v.y < comp.y);
}
bvec2 greaterEq(float2 v, float2 comp)
{
	return bvec2(v.x >= comp.x, v.y >= comp.y);
}
bvec2 lesserEq(float2 v, float2 comp)
{
	return bvec2(v.x <= comp.x, v.y <= comp.y);
}
