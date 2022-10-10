#ifndef COMPONENTS_INCLUDE_FILE_H
#define COMPONENTS_INCLUDE_FILE_H

#include "components_macro.h"

SER_DATA_BEGIN(Heritaged, 1, 1)
    INT_FIELD(TempInt, 10)
    FLOAT_FIELD(TempFloat, 20.0f)
SER_DATA_END()


SER_DATA_BEGIN(Heritaged2, 2, 1)
    INT_FIELD(TempInt2, 10)
    FLOAT_FIELD(TempFloat2, 20.0f)
SER_DATA_END()

#endif