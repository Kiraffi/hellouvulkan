
#include "gameentity.h"

#include <core/json.h>
#include <core/writejson.h>

bool findEntityType(const char *name, EntityType &outType)
{
    for(const auto entityName : nameStrings)
    {
        if(strcmp(name, entityName.name) == 0)
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
    json.addString("name", entity.name.getStr());
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

bool writeGameObject(const char *name, const GameEntity &entity, WriteJson &json)
{
    json.addObject(name);
    return writeGameObjectContent(entity, json);
}


bool loadGameObject(const JsonBlock &json, GameEntity &outEntity)
{
    int index = 0;
    std::string_view objTypeName;

    if(!json.getChild("magicNumber").equals(GameEntity::MagicNumber))
        return false;
    std::string_view name;
    if(!json.getChild("name").parseString(name))
        return false;

    if(!json.getChild("pos").parseVec3(outEntity.transform.pos))
        return false;
    if(!json.getChild("rot").parseQuat(outEntity.transform.rot))
        return false;
    if(!json.getChild("scale").parseVec3(outEntity.transform.scale))
        return false;

    if(!json.getChild("modelType").parseString(objTypeName))
        return false;

    if(!findEntityType(std::string(objTypeName).c_str(), outEntity.entityType))
        return false;

    outEntity.name = std::string(name).c_str();
    return true;
}