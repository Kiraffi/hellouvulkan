#pragma once

#include <core/json.h>
#include <core/mytypes.h>
#include <core/writejson.h>

#include <math/matrix.h>
#include <math/quaternion.h>
#include <math/vector3.h>

// For some reason windows didnt like forward declaring, or there was something missing.
//struct JsonBlock;
//struct WriteJson;

// Never delete any value, only add to this...
// Might be bit bad if multiple people were to modify at the same time
// or at least possibly cause problems.
enum class ComponentType : u32
{
    ComponentTypeNone = 0,
    TransformComponent,
    CameraComponent,
    Mat3x4Component,



    HeritagedType = 100'000,
    HeritagedType2,
    HeritagedType3,

    ComponentTypeCount
};

enum class EntitySystemType : u16
{
    EntitySystemTypeNone = 0,
    GameEntitySystemType,




    StaticModelEntitySystemType = 1'000,
    OtherTestEntitySystemType,

    EntitySystemTypeCount
};

struct EntitySystemHandle
{
    EntitySystemType entitySystemType = EntitySystemType::EntitySystemTypeCount;
    u16 entityIndexVersion = ~(u16(0u));
    u32 entityIndex = ~u32(0);
 };

struct EntityRWHandle
{
    u64 readArrays = 0;
    u64 writeArrays = 0;
    u32 syncIndexPoint = 0;
    EntitySystemType rwHandleTypeId = EntitySystemType::EntitySystemTypeCount;
};

/*
struct EnumType
{
    enum : i32
    {
        TestType,
        TestType2,

        EnumTypeCount
    }
    enumValue = EnumTypeCount;

    static constexpr char *const enumNames[] =
    {
        "TestType",
        "TestType2",

        "EnumTypeCount"
    };
};

struct EnumType2
{
    enum : u32
    {
        TestType2,
        TestType22,

        EnumTypeCount
    }
    enumValue = EnumTypeCount;

    static constexpr char *const enumNames[] =
    {
        "TestType2",
        "TestType22",

        "EnumTypeCount"
    };
};
*/

enum class FieldType
{
    EnumType,

    BoolType,

    I8Type,
    U8Type,
    I16Type,
    U16Type,
    I32Type,
    U32Type,
    I64Type,
    U64Type,

    F32Type,
    F64Type,

    Vec2Type,
    Vec3Type,
    Vec4Type,
    QuatType,

    Mat3x4Type,
    Mat4Type,

    NumTypes
};

bool serializeField(WriteJson &writeJson,
    const char* const fieldName,
    const void* const fieldMemoryAddress,
    FieldType fieldType);

bool deserializeField(const JsonBlock &json,
    const char* const fieldName,
    void* fieldMemoryAddress,
    FieldType fieldType);

void printFieldValue(const char* fieldName,
    const void* const fieldMemoryAddress,
    FieldType field);


void imguiPrintField(const char* fieldName,
    void* fieldMemoryAddress,
    FieldType field);

void imguiPrintField(const char* fieldName,
    void* fieldMemoryAddress,
    FieldType field,
    const char* const* enumNames, u32 enumCount);