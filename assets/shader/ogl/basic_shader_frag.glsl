
layout (location = 0) uniform sampler2D img;


layout (location = 0 ) in float2 st;


layout (location = 0) out float4 fc;


void main()
{
    //fc = texture(img, st);
    fc = float4(st, 0.0f, 1.0f);
}