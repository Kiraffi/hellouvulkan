
#include "scene.h"

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
    // animated first
    if (!loadModelForScene(sceneData, "assets/models/animatedthing.gltf", EntityType::WOBBLY_THING))
        return false;


    // load nonanimated.
    if (!loadModelForScene(sceneData, "assets/models/arrows.gltf", EntityType::ARROW))
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