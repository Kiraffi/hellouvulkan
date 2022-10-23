#ifndef COMPONENTS_INCLUDE_FILE_H
#define COMPONENTS_INCLUDE_FILE_H

#include "components_macro.h"
#include "../generated_header.h"

SER_DATA_BEGIN(Heritaged, ComponentType::HeritagedType, 1)
    INT_FIELD(TempInt, 10)
    FLOAT_FIELD(TempFloat, 20.0f)
    VEC2_FIELD(TempV2, 41, 520)
    VEC3_FIELD(TempV3, 10, 20, 30)
    VEC4_FIELD(TempV4, 7, 8, -30, 10)
SER_DATA_END(Heritaged)


SER_DATA_BEGIN(Heritaged2, ComponentType::HeritagedType2, 1)
    INT_FIELD(TempInt2, 10)
    FLOAT_FIELD(TempFloat2, 20.0f)
SER_DATA_END(Heritaged2)


SER_DATA_BEGIN(Heritaged3, ComponentType::HeritagedType3, 1)
    INT_FIELD(TempInt3, 10)
    INT_FIELD(TempInt4, 20)
    FLOAT_FIELD(TempFloat3, 20.0f)
SER_DATA_END(Heritaged3)


/*
class Entity
{
public:
    bool serialize();
    bool deserialize();

    const std::Vector<Type> getTypeVectorRead() const;
    const std::Vector<Type> getTypeVectorWrite();

private:
    std::vector<Type> ;
    std::vector<Type2>;

    std::atomic<u32>
}
*/
#endif