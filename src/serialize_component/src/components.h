#ifndef COMPONENTS_INCLUDE_FILE_H
#define COMPONENTS_INCLUDE_FILE_H

#include "components_macro.h"


#ifndef COMPONENTS_INCLUDE_FILE_H_ONCE
#define COMPONENTS_INCLUDE_FILE_H_ONCE

// Never delete any value, only add to this
enum DataTypes
{
    HeritagedType,
    HeritagedType2,
    HeritagedType3,


    DataTypeCount
};
#endif

SER_DATA_BEGIN(Heritaged, DataTypes::HeritagedType, 1)
    INT_FIELD(TempInt, 10)
    FLOAT_FIELD(TempFloat, 20.0f)
SER_DATA_END()


SER_DATA_BEGIN(Heritaged2, DataTypes::HeritagedType2, 1)
    INT_FIELD(TempInt2, 10)
    FLOAT_FIELD(TempFloat2, 20.0f)
SER_DATA_END()


SER_DATA_BEGIN(Heritaged3, DataTypes::HeritagedType3, 1)
    INT_FIELD(TempInt3, 10)
    INT_FIELD(TempInt4, 20)
    FLOAT_FIELD(TempFloat3, 20.0f)
SER_DATA_END()

#endif