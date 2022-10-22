#pragma once

// This is generated file, do not modify.

#include "src/components_macro.h"
#include <math/vector3.h>

struct Heritaged31
{
    static constexpr const char* componentName = "Heritaged31";
    static constexpr unsigned int componentID = DataTypes::HeritagedType;
    static constexpr unsigned int componentVersion = 1;
    static constexpr unsigned int ComponentFieldAmount = 5;

    // Row 0, Size 4
    int TempInt = 10;
    // Row 1, Size 4
    float TempFloat = 20.0f;
    // Row 2, Size 8
    Vector2 TempV2 = {41, 520};
    // Row 3, Size 12
    Vector3 TempV3 = {10, 20, 30};
    // Row 4, Size 16
    Vector4 TempV4 = {7, 8, -30, 10};

    static constexpr FieldType fieldTypes[] =
    {
        FieldType::IntType,
        FieldType::FloatType,
        FieldType::Vec2Type,
        FieldType::Vec3Type,
        FieldType::Vec4Type,
    };

    static constexpr const char* fieldNames[] =
    {
        "TempInt",
        "TempFloat",
        "TempV2",
        "TempV3",
        "TempV4",
    };

    void* getElementIndex(unsigned int index)
    {
        switch(index)
        {
            case 0: return &TempInt;
            case 1: return &TempFloat;
            case 2: return &TempV2;
            case 3: return &TempV3;
            case 4: return &TempV4;

            default: ASSERT_STRING(false, "Unknown index");
        }
        return nullptr;
    }
};

struct Heritaged21
{
    static constexpr const char* componentName = "Heritaged21";
    static constexpr unsigned int componentID = DataTypes::HeritagedType2;
    static constexpr unsigned int componentVersion = 1;
    static constexpr unsigned int ComponentFieldAmount = 2;

    // Row 0, Size 4
    int TempInt2 = 10;
    // Row 1, Size 4
    float TempFloat2 = 20.0f;

    static constexpr FieldType fieldTypes[] =
    {
        FieldType::IntType,
        FieldType::FloatType,
    };

    static constexpr const char* fieldNames[] =
    {
        "TempInt2",
        "TempFloat2",
    };

    void* getElementIndex(unsigned int index)
    {
        switch(index)
        {
            case 0: return &TempInt2;
            case 1: return &TempFloat2;

            default: ASSERT_STRING(false, "Unknown index");
        }
        return nullptr;
    }
};
