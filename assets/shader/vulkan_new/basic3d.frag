#version 450 core

layout(early_fragment_tests) in;
//layout(origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormals;

layout (location = 0) in vec4 colIn;
layout (location = 1) in vec3 normalDirIn;
layout (location = 2) in flat uint attributes;
layout (location = 3) in vec2 uv;

layout (binding = 16) uniform sampler2D textureToSample;

void main()
{
    outColor.a = 1.0f;
    if((attributes & 1) == 1)
        outColor.rgb = colIn.rgb;
    else if((attributes & 2) == 2)
        outColor.rgba = textureLod(textureToSample, uv.xy, 0);
    outNormals.rgba = vec4(normalDirIn, 0.0f);
}