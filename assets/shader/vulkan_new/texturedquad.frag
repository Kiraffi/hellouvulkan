#version 450 core

layout(origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out vec4 outColor;

layout (location = 0) in vec4 colIn;
layout (location = 1) in vec2 uvIn;
layout(depth_unchanged) out float gl_FragDepth;

layout (binding = 2) uniform sampler2D ourTexture;

void main()
{
	outColor = texture(ourTexture, uvIn);
	outColor.a = clamp(outColor.a / 0.5f, 0.0f, 1.0f);
	if(outColor.a < 0.5f)
		discard;
	outColor.rgb = colIn.rgb;

	//outColor.a = 1.0f;
}