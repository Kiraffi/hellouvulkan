#pragma once

#include <container/stackstring.h>
#include <render/meshrendersystem.h>
#include <scene/gameentity.h>

#include <model/animation.h>

struct Ray;
struct HitPoint;

// this is bit bad access
struct SceneData
{
    PodVector<AnimationState> animationStates;
    PodVector<GameEntity> entities;
    PodVector<u32> freeEnityIndices;
};

class Scene
{
public:
    static constexpr u32 MagicNumber = 1385621965u;
    static constexpr u32 VersionNumber = 1u;

    bool init();
    bool update(double deltaTime);

    u32 castRay(const Ray &ray, HitPoint &hitpoint);

    u32 addGameEntity(const GameEntity& entity, const SmallStackString &str = "");
    GameEntity &getEntity(u32 index) const;
    Bounds getBounds(u32 entityIndex) const;

    const PodVector<GameEntity> &getEntities() const { return sceneData.entities; }
    PodVector<GameEntity> &getEntities() { return sceneData.entities; }

    // TODO should think where this code should live.
    u32 addAnimation(u32 entityIndex, const char *animName, PlayMode playMode);
    u32 addAnimation(u32 entityIndex, u32 animationIndex, PlayMode playMode);
    u32 replaceAnimation(u32 entityIndex, u32 animationIndex, u32 playingAnimatinIndex);
    u32 replaceAnimation(u32 entityIndex, const char *animName, u32 playingAnimatinIndex);

    bool readLevel(const char *levelName);
    bool writeLevel(const char *filename) const;

    const SmallStackString &getSceneName() const { return sceneName; }

private:
    SceneData sceneData;
    SmallStackString sceneName = "Scene";
};


