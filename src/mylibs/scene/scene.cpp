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

// FLT_MAX
#include <float.h>
#include <string>

static GameEntity ConstEntity{ .entityType = EntityType::NUM_OF_ENTITY_TYPES };

static bool loadModelForScene(SceneData &sceneData, const char *filename, EntityType entityType)
{
    if(uint32_t(entityType) >= uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return false;
    GltfModel& gltfModel = sceneData.models[uint32_t(entityType)];

    bool readSuccess = readGLTF(filename, gltfModel);

    printf("%s gltf read success: %i\n", filename, readSuccess);
    if (!readSuccess)
    {
        sceneData.models.removeIndex(sceneData.models.size() - 1);
        defragMemory();
        return false;
    }

    if(!sceneData.meshRenderSystem.addModel(gltfModel, entityType))
    {
        defragMemory();
        return false;
    }
    defragMemory();

    return true;
}


bool Scene::init()
{
    ScopedTimer timer("Scene init");
    sceneData.models.resize(uint32_t(EntityType::NUM_OF_ENTITY_TYPES) + 1);

    // load animated, can be loaded in any order nowadays, since animated vertices has separate buffer from non-animated ones
    if (!loadModelForScene(sceneData, "assets/models/animatedthing.gltf", EntityType::WOBBLY_THING))
        return false;
    if (!loadModelForScene(sceneData, "assets/models/character8.gltf", EntityType::CHARACTER))
        return false;
    if (!loadModelForScene(sceneData, "assets/models/lowpoly6.gltf", EntityType::LOW_POLY_CHAR))
        return false;
    if(!loadModelForScene(sceneData, "assets/models/armature_test.gltf", EntityType::ARMATURE_TEST))
        return false;
    if(!loadModelForScene(sceneData, "assets/models/character4_22.gltf", EntityType::NEW_CHARACTER_TEST))
        return false;



    // load nonanimated.
    if (!loadModelForScene(sceneData, "assets/models/arrows.gltf", EntityType::ARROW))
        return false;

    if (!loadModelForScene(sceneData, "assets/models/test_gltf.gltf", EntityType::TEST_THING))
        return false;

    if (!loadModelForScene(sceneData, "assets/models/tree1.gltf", EntityType::TREE))
        return false;

    if (!loadModelForScene(sceneData, "assets/models/tree1_smooth.gltf", EntityType::TREE_SMOOTH))
        return false;

    if (!loadModelForScene(sceneData, "assets/models/blob.gltf", EntityType::BLOB))
        return false;

    if (!loadModelForScene(sceneData, "assets/models/blob_flat.gltf", EntityType::BLOB_FLAT))
        return false;

    if (!loadModelForScene(sceneData, "assets/models/floor.gltf", EntityType::FLOOR))
        return false;


    return true;
}


bool Scene::update(double deltaTime)
{
    //ScopedTimer timer("anim update");
    // better pattern for memory when other array gets constantly resized, no need to recreate same temporary array.
    PodVector<Mat3x4> matrices;
    matrices.reserve(256);
    for (auto& entity : sceneData.entities)
    {
        matrices.clear();
        uint32_t renderMeshIndex = uint32_t(entity.entityType);
        if (entity.entityType >= EntityType::NUM_OF_ENTITY_TYPES)
            continue;
        if (renderMeshIndex >= sceneData.models.size())
            continue;

        const auto& model = sceneData.models[renderMeshIndex];
        if(model.vertices.size() == 0 && model.animationVertices.size() == 0)
            continue;
        if (model.animationVertices.size() > 0)
        {
            entity.animationTime += deltaTime;
            if (!evaluateAnimation(model, entity.animationIndex, entity.animationTime, matrices))
                continue;
        }

        Mat3x4 renderMatrix = getModelMatrix(entity.transform);
        Mat3x4 normalMatrix = getModelNormalMatrix(entity.transform);
        sceneData.meshRenderSystem.addModelToRender(renderMeshIndex, renderMatrix, normalMatrix, matrices);
    }
    return true;
}


uint32_t Scene::addGameEntity(const GameEntity& entity)
{
    uint32_t result = ~0u;
    if (uint32_t(entity.entityType) > uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return result;
    if(sceneData.freeEnityIndices.size() > 0)
    {
        result = sceneData.freeEnityIndices.popBack();
        sceneData.entities[result] = entity;
    }
    else
    {
        result = sceneData.entities.size();
        sceneData.entities.push_back(entity);
    }
    sceneData.entities[result].index = result;
    return result;
}

GameEntity &Scene::getEntity(uint32_t index) const
{
    ASSERT(index < sceneData.entities.size());
    if (index >= sceneData.entities.size())
        return ConstEntity;

    return sceneData.entities[index];
}

Bounds Scene::getBounds(uint32_t entityIndex) const
{
    Bounds result;
    if(entityIndex >= sceneData.entities.size())
        return result;

    const auto &entity = sceneData.entities[entityIndex];
    if(uint32_t(entity.entityType) > uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return result;

    const auto &model = sceneData.models[uint32_t(entity.entityType)];
    return model.bounds;
}



bool Scene::readLevel(const char *levelName)
{
    PodVector<char> buffer;

    if(!loadBytes(levelName, buffer))
        return false;

    JsonBlock json;
    bool parseSuccess = json.parseJson(ArraySliceView(buffer.data(), buffer.size()));

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

    std::string_view mapName;
    if(!json.getChild("levelName").parseString(mapName))
        return false;

    if(json.getChild("objects").getChildCount() == 0)
        return false;

    std::string_view objTypeName;
    PodVector<GameEntity> newEntities;
    for(auto const &obj : json.getChild("objects"))
    {
        GameEntity ent;
        if(!loadGameObject(obj, ent))
            return false;

        ent.index = newEntities.size();
        newEntities.push_back(ent);
    }
    sceneData.entities = newEntities;
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
        writeBytes(filename, ArraySliceViewBytes(writeJson.getString().data(), writeJson.getString().size()));
}

uint32_t Scene::castRay(const Ray &ray, HitPoint &outHitpoint)
{
    uint32_t result = ~0u;

    float closestDist = FLT_MAX;
    uint32_t index = 0;

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
        const auto &model = sceneData.models[uint32_t(entity.entityType)];

        HitPoint hitpoint{ Uninit };
        if(rayOOBBBoundsIntersect(ray, model.bounds, entity.transform, hitpoint))
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

