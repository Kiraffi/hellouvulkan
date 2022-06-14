#include "testfuncs.h"

#include <math/general_math.h>
#include <math/matrix.h>
#include <math/quaternion.h>

static void testIdentity()
{
    {
        Matrix m;
        ASSERT(isIdentity(m));
    }

    for(uint32_t i = 0; i < 16; ++i)
    {
        Matrix m;
        m[i] = 2.0f;
        ASSERT(!isIdentity(m));
    }
}

static void testMatrixMultiply()
{
    Matrix m1 = {
        2, 3, 5, 7,
        11, 13, 17, 19,
        23, 29, 31, 37,
        41, 43, 47, 53
    };

    Matrix m2 = {
        59, 61, 67, 71,
        73, 79, 83, 89,
        97, 101, 103, 107,
        109, 113, 127, 131
    };

    Matrix mulM = m1 * m2;
    printMatrix(mulM, "matrix multiply");

    ASSERT(mulM._00 == 1585);
    ASSERT(mulM._01 == 1655);
    ASSERT(mulM._02 == 1787);
    ASSERT(mulM._03 == 1861);

    ASSERT(mulM._10 == 5318);
    ASSERT(mulM._11 == 5562);
    ASSERT(mulM._12 == 5980);
    ASSERT(mulM._13 == 6246);

    ASSERT(mulM._20 == 10514);
    ASSERT(mulM._21 == 11006);
    ASSERT(mulM._22 == 11840);
    ASSERT(mulM._23 == 12378);

    ASSERT(mulM._30 == 15894);
    ASSERT(mulM._31 == 16634);
    ASSERT(mulM._32 == 17888);
    ASSERT(mulM._33 == 18710);
}

static void testMatrixMultiply3x4()
{
    Mat3x4 m1 = {
        2, 3, 5, 7,
        11, 13, 17, 19,
        23, 29, 31, 37,
    };

    Mat3x4 m2 = {
        59, 61, 67, 71,
        73, 79, 83, 89,
        97, 101, 103, 107,
    };

    Mat3x4 mulM = m1 * m2;
    Matrix mulMat = Matrix(m1) * Matrix(m2);

    printMatrix(mulM, "matrix3x4 multiply");
    printMatrix(mulMat, "matrix4x4 multiply of mat3x4");


    for(uint32_t i = 0; i < 12; ++i)
    {
        ASSERT(mulM[i] == mulMat[i]);
    }
}


void testQuaternion()
{
    Quat q = getQuaternionFromAxisAngle(Vector3(0, 1, 0), toRadians(45));
}


void testMatrixFromQuaternion()
{
    

}


void testMatrix()
{
    testIdentity();
    testMatrixMultiply();
    testMatrixMultiply3x4();
    testQuaternion();
    testMatrixFromQuaternion();
}