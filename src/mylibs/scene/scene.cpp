
#include "scene.h"

#include <core/timer.h>

static GameEntity ConstEntity{ .entityType = EntityType::NUM_OF_ENTITY_TYPES };


static bool loadModelForScene(SceneData &sceneData, std::string_view filename, EntityType entityType)
{
    sceneData.models.push_back(RenderModel{});
    RenderModel& renderModel = sceneData.models.back();

    bool readSuccess = readGLTF(filename, renderModel);

    printf("%s gltf read success: %i\n", filename.data(), readSuccess);
    if (!readSuccess)
    {
        sceneData.models.removeIndex(sceneData.models.size() - 1);
        return false;
    }

    sceneData.modelRenderMeshTypes.insert({ entityType, sceneData.meshRenderSystem.addModel(renderModel) });

    return true;
}


bool Scene::init()
{
    ScopedTimer timer("Scene init");

    // animated first, because it makes animatedvertices first so the vertex index is same, otherwise i need to somehow pass special offset
    // to vertex shader to determine where to read animated vertices.
    if (!loadModelForScene(sceneData, "assets/models/animatedthing.gltf", EntityType::WOBBLY_THING))
        return false;
    if (!loadModelForScene(sceneData, "assets/models/character8.gltf", EntityType::CHARACTER))
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


bool Scene::update(double currentTime)
{
    for (const auto& entity : sceneData.entities)
    {
        if (entity.entityType == EntityType::NUM_OF_ENTITY_TYPES)
            continue;
        if (uint32_t(entity.entityType) >= sceneData.models.size())
            continue;
        const auto iter = sceneData.modelRenderMeshTypes.find(entity.entityType);
        if (iter == sceneData.modelRenderMeshTypes.end())
            continue;

        const auto& model = sceneData.models[uint32_t(entity.entityType)];
        uint32_t renderMeshIndex = iter->second;

        PodVector<Matrix> matrices;

        if (model.animationVertices.size() > 0)
        {
            if (!evaluateAnimation(model, 0, currentTime, matrices))
                continue;
        }

        Matrix renderMatrix = getModelMatrix(entity.transform);
        sceneData.meshRenderSystem.addModelToRender(renderMeshIndex, renderMatrix, matrices);
    }
    return true;
}


uint32_t Scene::addGameEntity(const GameEntity& entity)
{
    if (uint32_t(entity.entityType) >= uint32_t(EntityType::NUM_OF_ENTITY_TYPES))
        return ~0u;

    uint32_t result = sceneData.entities.size();

    sceneData.entities.push_back(entity);
    return result;
}

GameEntity& Scene::getEntity(uint32_t index)
{
    ASSERT(index < sceneData.entities.size());
    if (index >= sceneData.entities.size())
        return ConstEntity;

    return sceneData.entities[index];
}