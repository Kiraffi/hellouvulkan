#include "animation.h"
 
#include <container/podvector.h>
#include <model/gltf.h>
#include <resources/globalresources.h>

uint32_t blendNewAnimation(AnimationState &animationState, uint8_t animationIndex, PlayMode playMode, float strength)
{
    uint32_t result = ~0u;
    // all AMOUNT channels are in use
    if(animationState.activeIndices >= (1u << AnimationState::AMOUNT) - 1u)
        return result;
    if(!globalResources || uint32_t(animationState.entityType) >= globalResources->models.size() )
        return result;
    const auto &model = globalResources->models[uint32_t(animationState.entityType)];
    if(animationIndex >= model.animNames.size())
        return result;
    for(uint32_t newIndex = 0u; newIndex < AnimationState::AMOUNT; ++newIndex)
    {
        bool isActive = ((animationState.activeIndices >> newIndex) & 1) == 1;
        if(!isActive)
        {
            animationState.activeIndices |= 1 << newIndex;
            animationState.blendValues[newIndex] = strength;
            animationState.playMode[newIndex] = playMode;

            animationState.time[newIndex] = model.animStartTimes[animationIndex];
            animationState.animationIndices[newIndex] = animationIndex;
            result = newIndex;
            return result;
        }
    }

    return result;
}

uint32_t replaceAnimation(AnimationState &animationState, uint8_t animationIndex, uint32_t oldIndex, float strength)
{
    uint32_t result = ~0u;
    if(oldIndex >= AnimationState::AMOUNT)
        return result;

    // If the channel isnt active
    if(((animationState.activeIndices >> oldIndex) & 1) == 0)
        return result;
    if(!globalResources || uint32_t(animationState.entityType) >= globalResources->models.size())
        return result;
    const auto &model = globalResources->models[uint32_t(animationState.entityType)];
    if(animationIndex >= model.animNames.size())
        return result;

    {
        animationState.activeIndices |= 1 << oldIndex;
        animationState.blendValues[oldIndex] = strength;

        animationState.time[oldIndex] = model.animStartTimes[animationIndex];
        animationState.animationIndices[oldIndex] = animationIndex;
        result = oldIndex;
    }
    return result;
}


void updateAnimations(AnimationState &animationState, float dt)
{
    if(!globalResources || uint32_t(animationState.entityType) >= globalResources->models.size())
        return;
    const auto &model = globalResources->models[uint32_t(animationState.entityType)];

    for(uint32_t i = 0; i < AnimationState::AMOUNT; ++i)
    {
        if((animationState.activeIndices >> i) & 1)
        {
            bool removeAnimation = false;
            uint32_t animIndex = animationState.animationIndices[i];
            if(animIndex >= model.animEndTimes.size())
            {
                removeAnimation = true;
            }
            else
            {
                animationState.time[i] += dt;
                float endTime = model.animEndTimes[animIndex];
                float startTime = model.animStartTimes[animIndex];
                float animDuration = endTime - startTime;

                bool finished = animationState.time[i] > endTime;

                while(animationState.time[i] > endTime)
                    animationState.time[i] -= animDuration;

                if(animationState.playMode[i] == PlayMode::PlayOnce && finished)
                    removeAnimation = true;
            }
            if(removeAnimation)
            {
                animationState.activeIndices &= ~(1u << i);
            }
        }
    }
}

bool evaluateAnimations(AnimationState &animationState, PodVector<Mat3x4> &outMatrices)
{
    if(!globalResources || uint32_t(animationState.entityType) >= globalResources->models.size())
        return false;
    const auto &model = globalResources->models[uint32_t(animationState.entityType)];
    return evaluateAnimation(model, animationState, outMatrices);
}
