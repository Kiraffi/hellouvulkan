layout (location = 0) in vec3 position;
layout (location = 1) in uint uv;

layout (location = 1) uniform mat4 transform;

layout (location = 0 ) out vec2 st;

void main()
{
    gl_Position = transform * vec4(position, 1.0f);
    float2 unpackUV = float2( (uv & 0xffff) / 65535.0f, (uv >> 16) / 65535.0f); 
    st = unpackUV;
}