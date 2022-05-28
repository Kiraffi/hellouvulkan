#pragma once

#include <render/meshsystem.h>
#include <scene/gameentity.h>

#include <unordered_map>

struct SceneData
{
    MeshRenderSystem& meshRenderSystem;

    Vector<RenderModel> models;
    PodVector<GameEntity> entities;
    std::unordered_map<EntityType, uint32_t> modelRenderMeshTypes;
};

class Scene
{
public:
    Scene(MeshRenderSystem& meshRenderSystem) : sceneData(SceneData{ .meshRenderSystem = meshRenderSystem } ) {}


    bool init();
    bool update(double currentTime);

    uint32_t addGameEntity(const GameEntity& entity);
    GameEntity& getEntity(uint32_t index);
private:
    SceneData sceneData;
};