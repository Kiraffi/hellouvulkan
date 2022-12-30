#define MATRIX_ORDER row_major
//#define MATRIX_ORDER column_major

layout(binding = 0, MATRIX_ORDER) uniform ConstantDataStructBlock
{
    vec2 windowSize;
    vec2 pad1;
    vec4 pad2;
    vec4 pad3;
    vec4 pad4;

    mat4 cameraMatrix;
    mat4 viewProjMat;
    mat4 mvp;
    mat4 sunMatrix;

    vec4 camPos;
    vec4 pad11;
    vec4 pad12;
    vec4 pad13;

    mat4 inverseMvp;
};
