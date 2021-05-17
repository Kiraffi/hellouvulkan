#version 450 core

layout(origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out vec4 col;

layout (location = 0) in vec4 colIn;
layout(depth_unchanged) out float gl_FragDepth;
	
void main()
{
	col = colIn;
}