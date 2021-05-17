#version 450

#include "shader_common_vulkan.glsl"
#include "common_buffers.glsl"

layout(location = 0) out float4 outputColor;

layout(location = 0) in float4 color;


/*
layout(binding = 0) readonly buffer buf
{
	float4 imageData[];
};
*/



void main()
{
	float2 p = gl_FragCoord.xy / gl_FragCoord.w / screenSize.xy; // (gl_FragCoord.xy / gl_FragCoord.w);
	p = fract(p);
	p = abs(1.0f - p * 2.0f);
	uint2 fragPos = uint2(p.xy * 1024.0f);
	uint pos = uint(fragPos.y * 1024 + fragPos.x);
	//float4 imageColor = imageData[pos];
	outputColor = float4(color.rgb, 1.0f);
	//outputColor = float4(imageColor.xy, gl_FragCoord.z, 1.0f); // color;
	//outputColor = float4(gl_FragCoord.xy / 1024.0f, gl_FragCoord.z, 1.0f); // color;
	//outputColor = float4(p.xy, abs(1.0f - 2.0f * fract(gl_FragCoord.z + padding.x / 1024.0f)), 1.0f); // color;
}