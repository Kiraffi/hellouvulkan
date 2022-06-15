#pragma once

#include <container/stackstring.h>
#include <render/meshrendersystem.h>
#include <scene/gameentity.h>

struct Ray;
struct HitPoint;

class MeshRenderSystem;

struct SceneData
{
    MeshRenderSystem& meshRenderSystem;

    Vector<GltfModel> models;
    PodVector<GameEntity> entities;
};

class Scene
{
public:
    static constexpr uint32_t MagicNumber = 1385621965u;
    static constexpr uint32_t VersionNumber = 1u;

    Scene(MeshRenderSystem& meshRenderSystem) : sceneData(SceneData{ .meshRenderSystem = meshRenderSystem } ) {}


    bool init();
    bool update(double deltaTime);

    uint32_t castRay(const Ray &ray, HitPoint &hitpoint);

    uint32_t addGameEntity(const GameEntity& entity);
    GameEntity &getEntity(uint32_t index) const;

    const PodVector<GameEntity> &getEntities() const { return sceneData.entities; }
    PodVector<GameEntity> &getEntities() { return sceneData.entities; }

    bool readLevel(std::string_view levelName);
    bool writeLevel(std::string_view filename) const;

    const SmallStackString &getSceneName() const { return sceneName; }
private:
    SceneData sceneData;

    SmallStackString sceneName = "Scene";
};


