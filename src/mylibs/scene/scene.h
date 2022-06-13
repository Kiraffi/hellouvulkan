#pragma once

#include <render/meshrendersystem.h>
#include <scene/gameentity.h>
struct SceneData
{
    MeshRenderSystem& meshRenderSystem;

    Vector<GltfModel> models;
    PodVector<GameEntity> entities;
};

class Scene
{
public:
    Scene(MeshRenderSystem& meshRenderSystem) : sceneData(SceneData{ .meshRenderSystem = meshRenderSystem } ) {}


    bool init();
    bool update(double deltaTime);

    uint32_t addGameEntity(const GameEntity& entity);
    GameEntity& getEntity(uint32_t index);
private:
    SceneData sceneData;
};