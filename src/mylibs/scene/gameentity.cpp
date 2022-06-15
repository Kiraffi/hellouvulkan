
#include "gameentity.h"

#include <core/writejson.h>

bool findEntityType(std::string_view name, EntityType &outType)
{
    for(const auto entityName : nameStrings)
    {
        if(name == entityName.name)
        {
            outType = entityName.type;
            return true;
        }
    }
    return false;
}

const char *getStringFromEntityType(const EntityType &type)
{
    for(const auto entityName : nameStrings)
    {
        if(type == entityName.type)
        {
            return entityName.name;
        }
    }
    ASSERT(false && "Type name not defined!");
    return "";
}

static bool writeGameObjectContent(const GameEntity &entity, WriteJson &json)
{
    json.addMagicNumberAndVersion(GameEntity::MagicNumber, GameEntity::VersionNumber);
    writeTransform(entity.transform, json);
    json.addString("modelType", getStringFromEntityType(entity.entityType));
    json.endObject();
    return json.isValid();
}

bool writeGameObject(const GameEntity &entity, WriteJson &json)
{
    json.addObject();
    return writeGameObjectContent(entity, json);
}

bool writeGameObject(std::string_view name, const GameEntity &entity, WriteJson &json)
{
    json.addObject(name);
    return writeGameObjectContent(entity, json);
}
