#pragma once

#include <resources/animationresource.h>
#include <container/podvector.h>


enum class AnimaitionBlendOp
{
    Add,
    Override
};

struct AnimationState
{
    float timeNormalized = 0.0f;
    uint32_t animationIndex = ~0u;
    PlayMode playMode = PlayMode::PlayOnce;
    // uint32_t loopCount = 0; // maybe?
};

struct Animation
{
    PodVector<AnimationState> runningAnimations;
    PodVector<float> blendValues;
};

void blendNewAnimation(Animation &animation, uint32_t animationIndex, AnimaitionBlendOp blendOp);

