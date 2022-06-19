#pragma once

#include <components/transform.h>

#include <container/stackstring.h>

#include <core/mytypes.h>

#include <math/bounds.h>
#include <math/quaternion.h>
#include <math/vector3.h>

class WriteJson;
class JsonBlock;

enum class EntityType : uint32_t
{
    // animated thingies
    WOBBLY_THING,
    CHARACTER,
    LOW_POLY_CHAR,
    ARMATURE_TEST,

    // non animated
    ARROW,
    TEST_THING,

    TREE,
    TREE_SMOOTH,

    BLOB,
    BLOB_FLAT,

    FLOOR,

    NUM_OF_ENTITY_TYPES,
};

struct EntityNameString
{
    EntityType type;
    const char *name;
};

static constexpr EntityNameString nameStrings[]
{
    EntityNameString{ .type = EntityType::WOBBLY_THING, .name = "Wobbly" },
    EntityNameString{ .type = EntityType::CHARACTER, .name = "Character" },
    EntityNameString{ .type = EntityType::LOW_POLY_CHAR, .name = "LowPolyChar" },
    EntityNameString{ .type = EntityType::ARMATURE_TEST, .name = "ArmatureTest" },

    EntityNameString{ .type = EntityType::ARROW, .name = "Arrow" },
    EntityNameString{ .type = EntityType::TEST_THING, .name = "TestThing" },

    EntityNameString{ .type = EntityType::TREE, .name = "Tree" },
    EntityNameString{ .type = EntityType::TREE_SMOOTH, .name = "TreeSmooth" },

    EntityNameString{ .type = EntityType::BLOB, .name = "Blob" },
    EntityNameString{ .type = EntityType::BLOB_FLAT, .name = "BlobFlat" },

    EntityNameString{ .type = EntityType::FLOOR, .name = "Floor" },
    EntityNameString{ .type = EntityType::NUM_OF_ENTITY_TYPES, .name = "Unknown" },
};

struct GameEntity
{
    static constexpr uint32_t MagicNumber = 9084352;
    static constexpr uint32_t VersionNumber = 1u;
    SmallStackString name = "Name";
    Transform transform;

    double animationTime = 0.0;
    uint32_t animationIndex = 0u;
    EntityType entityType = EntityType::NUM_OF_ENTITY_TYPES;
    uint32_t index = 0;
};

bool writeGameObject(const char *name, const GameEntity &entity, WriteJson &json);
bool writeGameObject(const GameEntity &entity, WriteJson &json);
bool loadGameObject(const JsonBlock &json, GameEntity &outEntity);
bool findEntityType(const char *name, EntityType &outType);
const char *getStringFromEntityType(const EntityType &type);

