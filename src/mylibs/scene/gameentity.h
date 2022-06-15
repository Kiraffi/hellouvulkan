#pragma once

#include <components/transform.h>

#include <core/mytypes.h>
#include <math/bounds.h>
#include <math/quaternion.h>
#include <math/vector3.h>

#include <string_view>

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
};

struct GameEntity
{
    Transform transform;
    Bounds bounds;
    double animationTime = 0.0;
    uint32_t animationIndex = 0u;
    EntityType entityType = EntityType::NUM_OF_ENTITY_TYPES;
};


bool findEntityType(std::string_view name, EntityType &outType);