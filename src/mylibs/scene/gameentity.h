#pragma once

#include <core/mytypes.h>
#include <core/transform.h>
#include <math/quaternion.h>
#include <math/vector3.h>


enum class EntityType : uint32_t
{
    // animated thingies
    WOBBLY_THING,

    // non animated
    ARROW,

    NUM_OF_ENTITY_TYPES,
};

struct GameEntity
{
    Transform transform;
    EntityType entityType = EntityType::NUM_OF_ENTITY_TYPES;
};