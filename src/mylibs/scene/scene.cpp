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


static GameEntity ConstEntity{ .entityType = EntityType::NUM_OF_ENTITY_TYPES };

static bool loadModelForScene(SceneData &sceneData, std::string_view filename, EntityType entityType)
{
    if(uint32_t(entityType) >= uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return false;
    GltfModel& gltfModel = sceneData.models[uint32_t(entityType)];

    bool readSuccess = readGLTF(filename, gltfModel);

    printf("%s gltf read success: %i\n", filename.data(), readSuccess);
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
    sceneData.models.resize(uint32_t(EntityType::NUM_OF_ENTITY_TYPES));

    // load animated, can be loaded in any order nowadays, since animated vertices has separate buffer from non-animated ones
    if (!loadModelForScene(sceneData, "assets/models/animatedthing.gltf", EntityType::WOBBLY_THING))
        return false;
    if (!loadModelForScene(sceneData, "assets/models/character8.gltf", EntityType::CHARACTER))
        return false;
    if (!loadModelForScene(sceneData, "assets/models/lowpoly6.gltf", EntityType::LOW_POLY_CHAR))
        return false;

    if(!loadModelForScene(sceneData, "assets/models/armature_test.gltf", EntityType::ARMATURE_TEST))
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
        if (entity.entityType == EntityType::NUM_OF_ENTITY_TYPES)
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
        sceneData.meshRenderSystem.addModelToRender(renderMeshIndex, renderMatrix, matrices);
    }
    return true;
}


uint32_t Scene::addGameEntity(const GameEntity& entity)
{
    uint32_t result = ~0u;
    if (uint32_t(entity.entityType) >= uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return result;
    const auto &model = sceneData.models[uint32_t(entity.entityType)];
    result = sceneData.entities.size();

    sceneData.entities.push_back(entity);
    sceneData.entities.back().bounds = model.bounds;
    sceneData.entities.back().index = result;
    return result;
}

GameEntity &Scene::getEntity(uint32_t index) const
{
    ASSERT(index < sceneData.entities.size());
    if (index >= sceneData.entities.size())
        return ConstEntity;

    return sceneData.entities[index];
}


bool Scene::readLevel(std::string_view levelName)
{
    PodVector<char> buffer;

    if(!loadBytes(levelName, buffer))
        return false;

    JSONBlock bl;
    bool parseSuccess = bl.parseJSON(ArraySliceView(buffer.data(), buffer.size()));

    if(!parseSuccess)
    {
        printf("Failed to parse: %s\n", levelName.data());
        return false;
    }
    else
    {
        //bl.print();
    }

    if(!bl.isObject() || bl.getChildCount() < 1)
        return false;

    uint32_t magicNumber;
    if(!bl.getChild("magicNumber").parseUInt(magicNumber))
        return false;
    if(magicNumber != Scene::MagicNumber)
        return false;

    uint32_t versionNumber;
    if(!bl.getChild("versionNumber").parseUInt(versionNumber))
        return false;

    std::string mapName;
    if(!bl.getChild("levelName").parseString(levelName))
        return false;

    if(bl.getChild("objects").getChildCount() == 0)
        return false;

    std::string_view objTypeName;
    PodVector<GameEntity> newEntities;
    for(auto const &obj : bl.getChild("objects"))
    {
        GameEntity ent;
        int index = 0;
        if(!obj.getChild("position").parseVec3(ent.transform.pos))
            return false;
        if(!obj.getChild("rotation").parseQuat(ent.transform.rot))
            return false;
        if(!obj.getChild("scale").parseVec3(ent.transform.scale))
            return false;

        if(!obj.getChild("modelType").parseString(objTypeName))
            return false;

        if(!findEntityType(objTypeName, ent.entityType))
            return false;
        const auto &model = sceneData.models[uint32_t(ent.entityType)];
        ent.bounds = model.bounds;
        ent.index = newEntities.size();
        newEntities.push_back(ent);
    }
    sceneData.entities = newEntities;
    return true;
}

bool Scene::writeLevel(std::string_view filename) const
{
    WriteJson writeJson(Scene::MagicNumber, Scene::VersionNumber);
    writeJson.addString("levelName", "Testmap");
    writeJson.addArray("objects");
    for(const auto &entity : sceneData.entities)
        writeGameObject(entity, writeJson);
    writeJson.endArray();
    writeJson.finishWrite();
    return writeJson.isValid();
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
        HitPoint hitpoint{ Uninit };
        if(rayOOBBBoundsIntersect(ray, entity.bounds, entity.transform, hitpoint))
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

