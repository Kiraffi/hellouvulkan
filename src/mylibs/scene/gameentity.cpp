
#include "gameentity.h"

#include <container/string.h>

#include <core/json.h>
#include <core/writejson.h>

#include <resources/globalresources.h>

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
    if(!globalResources)
        return false;
    if(uint32_t(entity.entityType) >= globalResources->models.size())
        return false;
    const auto &model = globalResources->models[uint32_t(entity.entityType)];
    json.addMagicNumberAndVersion(GameEntity::MagicNumber, GameEntity::VersionNumber);
    json.addString("name", entity.name.getStr());
    writeTransform(entity.transform, json);
    json.addString("modelType", getStringFromEntityType(entity.entityType));
    if(entity.meshIndex < model.modelMeshes.size() && model.modelMeshes[entity.meshIndex].meshName.getSize() > 0)
        json.addString("mesh", model.modelMeshes[entity.meshIndex].meshName.getStr());
    if(entity.animationIndex < model.animNames.size())
        json.addString("anim", model.animNames[entity.animationIndex].getStr());
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
    StringView objTypeName;

    if(!json.getChild("magicNumber").equals(GameEntity::MagicNumber))
        return false;
    StringView name;
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

    if(!findEntityType(String(objTypeName.ptr, objTypeName.length).getStr(), outEntity.entityType))
        return false;

    StringView meshName;
    bool foundMesh = json.getChild("mesh").parseString(meshName);

    StringView animName;
    bool foundAnim = json.getChild("anim").parseString(animName);


    outEntity.meshIndex = 0u;
    outEntity.animationIndex = 0u;

    if(uint32_t(outEntity.entityType) < globalResources->models.size())
    {
        const auto &model = globalResources->models[uint32_t(outEntity.entityType)];
        if(foundMesh)
        {
            for(uint32_t meshIndex = 0u; meshIndex < model.modelMeshes.size(); ++meshIndex)
            {
                if(model.modelMeshes[meshIndex].meshName == meshName.data())
                {
                    outEntity.meshIndex = meshIndex;
                    break;
                }
            }
        }
        if(foundAnim)
        {
            for(uint32_t animIndex = 0u; animIndex < model.animNames.size(); ++animIndex)
            {
                if(model.animNames[animIndex] == animName.data())
                {
                    outEntity.animationIndex = animIndex;
                    break;
                }
            }
        }

    }

    outEntity.name = String(name.ptr, name.length).getStr();
    return true;
}