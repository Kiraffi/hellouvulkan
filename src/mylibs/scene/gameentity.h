#pragma once

#include <core/mytypes.h>
#include <core/transform.h>
#include <math/bounds.h>
#include <math/quaternion.h>
#include <math/vector3.h>


enum class EntityType : uint32_t
{
    // animated thingies
    WOBBLY_THING,
    CHARACTER,
    LOW_POLY_CHAR,

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

struct GameEntity
{
    Transform transform;
    Bounds bounds;
    double animationTime = 0.0;
    uint32_t animationIndex = 0u;
    EntityType entityType = EntityType::NUM_OF_ENTITY_TYPES;
};