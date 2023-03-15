#pragma once

struct GpuFrameBuffer
{
    Vector2 windowSize;
    Vector2 tmp1;
    Vector4 tmp2[3];

    Matrix camMat;
    Matrix viewProj;
    Matrix mvp;
    Matrix sunMatrix;


    Vector4 camPos;
    Vector4 tmp3[3];

    Matrix inverseMvp;
};
