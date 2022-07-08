#pragma once

#include <resources/animationresource.h>
#include <scene/gameentity.h>

template<typename T>
class PodVector;

struct AnimationState
{
    static constexpr uint32_t AMOUNT = 4;
    float blendValues[AMOUNT] = {};
    float time[AMOUNT] = {};
    uint8_t animationIndices[AMOUNT] = { 255u, 255u, 255u, 255u };
    PlayMode playMode[AMOUNT] = { PlayMode::PlayOnce, PlayMode::PlayOnce, PlayMode::PlayOnce, PlayMode::PlayOnce };
    EntityType entityType = EntityType::NUM_OF_ENTITY_TYPES;
    uint32_t activeIndices = 0u;
};


uint32_t replaceAnimation(AnimationState &animationState, uint8_t animationIndex, uint32_t oldIndex, float strength);

uint32_t blendNewAnimation(AnimationState &animationState, uint8_t animationIndex, PlayMode playMode, float strength);
bool evaluateAnimations(AnimationState &animationState, PodVector<Mat3x4> &outMatrices);
void updateAnimations(AnimationState &animationState, float dt);
