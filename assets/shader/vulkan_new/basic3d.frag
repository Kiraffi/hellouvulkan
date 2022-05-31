#version 450 core

//layout(origin_upper_left) in vec4 gl_FragCoord;
layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormals;

layout (location = 0) in vec4 colIn;
layout (location = 1) in vec3 normalDirIn;
void main()
{
    outColor.a = 1.0f;
    outColor.rgb = colIn.rgb;

    outNormals.rgba = vec4(normalDirIn, 0.0f);
}