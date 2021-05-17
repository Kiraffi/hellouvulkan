#version 450 core

layout(origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out vec4 outColor;

layout (location = 0) in flat vec4 colIn;
layout(depth_unchanged) out float gl_FragDepth;

void main()
{
	outColor.a = 1.0f;
	outColor.rgb = colIn.rgb;
}