#pragma once

#include <core/mytypes.h>

struct JsonBlock;
struct WriteJson;

// Never delete any value, only add to this...
// Might be bit bad if multiple people were to modify at the same time
// or at least possibly cause problems.
enum class ComponentType : u32
{
    HeritagedType = 1,
    HeritagedType2,
    HeritagedType3,

    ComponentTypeCount
};

enum class EntityType : u16
{
    StaticModelEntityType = 1,

    EntityTypeCount
};

struct EntitySystemHandle
{
    EntityType entitySystemType = EntityType::EntityTypeCount;
    u16 entityIndexVersion = ~u16(0);
    u32 entityIndex = ~u32(0);
 };

enum class FieldType
{
    IntType,
    FloatType,

    Vec2Type,
    Vec3Type,
    Vec4Type,

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
