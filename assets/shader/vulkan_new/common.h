#define MATRIX_ORDER row_major
//#define MATRIX_ORDER column_major

layout(binding = 0, MATRIX_ORDER) uniform ConstantDataStructBlock
{
    mat4 cameraMatrix;
    mat4 viewProjMat;
    mat4 mvp;
    mat4 sunMatrix;

    vec2 windowSize;
    vec2 pad1;

    vec4 camPos;
    vec4 pad3;
    vec4 pad4;

    mat4 inverseMvp;
};
