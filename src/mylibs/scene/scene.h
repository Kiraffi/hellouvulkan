#pragma once

#include <container/stackstring.h>
#include <render/meshrendersystem.h>
#include <scene/gameentity.h>

#include <model/animation.h>

struct Ray;
struct HitPoint;

class MeshRenderSystem;

// this is bit bad access
struct SceneData
{
    MeshRenderSystem& meshRenderSystem;

    PodVector<AnimationState> animationStates;
    PodVector<GameEntity> entities;
    PodVector<uint32_t> freeEnityIndices;
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

    uint32_t addGameEntity(const GameEntity& entity, const SmallStackString &str = "");
    GameEntity &getEntity(uint32_t index) const;
    Bounds getBounds(uint32_t entityIndex) const;

    const PodVector<GameEntity> &getEntities() const { return sceneData.entities; }
    PodVector<GameEntity> &getEntities() { return sceneData.entities; }
    
    // TODO should think where this code should live.
    uint32_t addAnimation(uint32_t entityIndex, const char *animName, PlayMode playMode);
    uint32_t addAnimation(uint32_t entityIndex, uint32_t animationIndex, PlayMode playMode);
    uint32_t replaceAnimation(uint32_t entityIndex, uint32_t animationIndex, uint32_t playingAnimatinIndex);
    uint32_t replaceAnimation(uint32_t entityIndex, const char *animName, uint32_t playingAnimatinIndex);

    bool readLevel(const char *levelName);
    bool writeLevel(const char *filename) const;

    const SmallStackString &getSceneName() const { return sceneName; }

private:
    SceneData sceneData;
    SmallStackString sceneName = "Scene";
};


