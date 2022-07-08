#include "scene.h"

#include <components/transform_functions.h>

#include <core/file.h>
#include <core/general.h>
#include <core/json.h>
#include <core/timer.h>
#include <core/writejson.h>

#include <math/hitpoint.h>
#include <math/ray.h>
#include <math/vector3_inline_functions.h>

#include <render/meshrendersystem.h>
#include <resources/globalresources.h>

// FLT_MAX
#include <float.h>

static GameEntity ConstEntity{ .entityType = EntityType::NUM_OF_ENTITY_TYPES };

static uint32_t getAnimationIndexFromName(const char *animationName, ArraySliceView<GameEntity> entities, uint32_t entityIndex)
{
    uint32_t result = ~0u;
    if(!globalResources)
        return result;

    if(entityIndex >= entities.size())
        return result;

    const auto &entity = entities[entityIndex];
    if(uint32_t(entity.entityType) > uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return result;

    const auto &model = globalResources->models[(uint32_t)entity.entityType];
    result = model.animNames.find(animationName);
    return result;
}


bool Scene::init()
{
    ScopedTimer timer("Scene init");

    ASSERT(globalResources);
    ASSERT(globalResources->models.size() != uint32_t(EntityType::NUM_OF_ENTITY_TYPES));

    for(uint32_t i = 0; i < uint32_t(EntityType::NUM_OF_ENTITY_TYPES); ++i)
    {
        if(!sceneData.meshRenderSystem.addModel(globalResources->models[i], EntityType(i)))
        {
            defragMemory();
            return false;
        }
    }

    return true;
}


bool Scene::update(double deltaTime)
{
    ASSERT(globalResources);

    //ScopedTimer timer("anim update");
    // better pattern for memory when other array gets constantly resized, no need to recreate same temporary array.
    PodVector<Mat3x4> matrices;
    matrices.reserve(256);
    uint32_t entityIndex = 0u;
    for (auto& entity : sceneData.entities)
    {
        matrices.clear();
        uint32_t renderMeshIndex = uint32_t(entity.entityType);
        if (entity.entityType >= EntityType::NUM_OF_ENTITY_TYPES)
            continue;
        if (renderMeshIndex >= globalResources->models.size())
            continue;

        const auto &model = globalResources->models[renderMeshIndex];
        if(entity.meshIndex >= model.modelMeshes.size())
            continue;

        const auto &mesh = model.modelMeshes[entity.meshIndex];

        if(mesh.vertices.size() == 0 && mesh.animationVertices.size() == 0)
            continue;
        if(mesh.animationVertices.size() > 0)
        {
            auto &state = sceneData.animationStates[entityIndex];
            if(state.activeIndices == 0)
            {
                entity.animationTime += deltaTime;
                if(!evaluateAnimation(model, entity.animationIndex, entity.animationTime, matrices))
                    continue;
            }
            else
            {
                updateAnimations(state, deltaTime);
                if(!evaluateAnimations(state, matrices))
                    continue;
            }
        }

        Mat3x4 renderMatrix = getModelMatrix(entity.transform);
        Mat3x4 normalMatrix = getModelNormalMatrix(entity.transform);
        sceneData.meshRenderSystem.addModelToRender(renderMeshIndex, renderMatrix, normalMatrix, matrices);
        ++entityIndex;
    }
    return true;
}


uint32_t Scene::addGameEntity(const GameEntity& entity, const SmallStackString &str)
{
    uint32_t result = ~0u;
    if(!globalResources)
        return result;

    if (uint32_t(entity.entityType) > uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return result;
    if(uint32_t(entity.entityType) >= globalResources->models.size())
        return result;

    const auto &model = globalResources->models[uint32_t(entity.entityType)];

    uint32_t meshIndex = 0u;

    if(str.getSize() > 0)
    {
        for(const auto &mesh : model.modelMeshes)
        {
            if(mesh.meshName == str)
                break;

            ++meshIndex;
        }
        if(meshIndex == model.modelMeshes.size())
            return result;
    }

    if(sceneData.freeEnityIndices.size() > 0)
    {
        result = sceneData.freeEnityIndices.popBack();
        sceneData.entities[result] = entity;
    }
    else
    {
        result = sceneData.entities.size();
        sceneData.entities.push_back(entity);
        sceneData.animationStates.pushBack(AnimationState());
    }
    sceneData.entities[result].index = result;
    sceneData.animationStates[result] = AnimationState();
    sceneData.animationStates[result].entityType = entity.entityType;

    return result;
}

GameEntity &Scene::getEntity(uint32_t index) const
{
    ASSERT(index < sceneData.entities.size());
    if (index >= sceneData.entities.size())
        return ConstEntity;

    return sceneData.entities[index];
}

uint32_t Scene::addAnimation(uint32_t entityIndex, const char *animName, PlayMode playMode)
{
    uint32_t animationIndex = getAnimationIndexFromName(animName, sliceFromPodVector(sceneData.entities), entityIndex);
    if(animationIndex != ~0u)
    {
        auto &animState = sceneData.animationStates[entityIndex];
        return blendNewAnimation(animState, animationIndex, playMode, 1.0f);
    }
    return ~0u;
}

uint32_t Scene::replaceAnimation(uint32_t entityIndex, const char *animName, uint32_t playingAnimatinIndex)
{
    uint32_t animationIndex = getAnimationIndexFromName(animName, sliceFromPodVector(sceneData.entities), entityIndex);
    if(animationIndex != ~0u)
    {
        auto &animState = sceneData.animationStates[entityIndex];
        return ::replaceAnimation(animState, animationIndex, playingAnimatinIndex, 1.0f);
    }
    return ~0u;
}


// TODO need to actually separate some entity systems from scene...
uint32_t Scene::addAnimation(uint32_t entityIndex, uint32_t animationIndex, PlayMode playMode)
{
    uint32_t result = ~0u;
    if(entityIndex >= sceneData.entities.size())
        return result;

    const auto &entity = sceneData.entities[entityIndex];
    if(uint32_t(entity.entityType) > uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return result;

    const auto &model = globalResources->models[(uint32_t)entity.entityType];
    if(animationIndex >= model.animNames.size())
        return result;

    auto &animState = sceneData.animationStates[entityIndex];
    return blendNewAnimation(animState, animationIndex, playMode, 1.0f);

}

Bounds Scene::getBounds(uint32_t entityIndex) const
{
    Bounds result;
    if(entityIndex >= sceneData.entities.size())
        return result;

    const auto &entity = sceneData.entities[entityIndex];
    if(uint32_t(entity.entityType) > uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return result;

    const auto &model = globalResources->models[uint32_t(entity.entityType)];
    if(model.modelMeshes.size() > 0)
        return model.modelMeshes[0].bounds;
    return Bounds();
}



bool Scene::readLevel(const char *levelName)
{
    PodVector<char> buffer;

    if(!loadBytes(levelName, buffer.getBuffer()))
        return false;

    JsonBlock json;
    bool parseSuccess = json.parseJson(StringView(buffer.data(), buffer.size()));

    if(!parseSuccess)
    {
        printf("Failed to parse: %s\n", levelName);
        return false;
    }
    else
    {
        //json.print();
    }

    if(!json.isObject() || json.getChildCount() < 1)
        return false;

    if(!json.getChild("magicNumber").equals(Scene::MagicNumber))
        return false;

    uint32_t versionNumber;
    if(!json.getChild("versionNumber").parseUInt(versionNumber))
        return false;

    StringView mapName;
    if(!json.getChild("levelName").parseString(mapName))
        return false;

    if(json.getChild("objects").getChildCount() == 0)
        return false;

    StringView objTypeName;

    PodVector<AnimationState> newAnimationStates;
    PodVector<GameEntity> newEntities;
    for(auto const &obj : json.getChild("objects"))
    {
        GameEntity ent;
        if(!loadGameObject(obj, ent))
            return false;

        ent.index = newEntities.size();
        newEntities.push_back(ent);
        newAnimationStates.push_back(AnimationState());
        newAnimationStates[newAnimationStates.size() - 1].entityType = ent.entityType;
    }
    sceneData.entities = newEntities;
    sceneData.animationStates = newAnimationStates;
    return true;
}

bool Scene::writeLevel(const char *filename) const
{
    WriteJson writeJson(Scene::MagicNumber, Scene::VersionNumber);
    writeJson.addString("levelName", sceneName.getStr());
    writeJson.addArray("objects");
    for(const auto &entity : sceneData.entities)
        writeGameObject(entity, writeJson);
    writeJson.endArray();
    writeJson.finishWrite();
    return writeJson.isValid() &&
        writeBytes(filename, writeJson.getString().getBuffer());
}

uint32_t Scene::castRay(const Ray &ray, HitPoint &outHitpoint)
{
    uint32_t result = ~0u;

    float closestDist = FLT_MAX;
    uint32_t index = 0;
    const auto models = sliceFromVector(globalResources->models);
    for(const auto &entity : sceneData.entities)
    {
        /*
        if(raySphereIntersect(ray, Sphere{ .pos = entity.transform.pos, .radius = 2.0f }, hitpoint))
        {
            float dist = sqrLen(hitpoint.point - ray.pos);
            if(dist < closestDist)
            {
                selectedEntityIndex = index;
                closestDist = dist;
                lineTo = hitpoint.point;
            }
        }
        */
        const auto &model = models[uint32_t(entity.entityType)];

        HitPoint hitpoint{ Uninit };
        Bounds bounds;
        if(entity.meshIndex < model.modelMeshes.size())
            bounds = model.modelMeshes[entity.meshIndex].bounds;
        if(rayOOBBBoundsIntersect(ray, bounds, entity.transform, hitpoint))
        {
            float dist = sqrLen(hitpoint.point - ray.pos);
            if(dist < closestDist)
            {
                result = index;
                closestDist = dist;
                outHitpoint = hitpoint;
            }
        }
        ++index;
    }

    return result;
}

