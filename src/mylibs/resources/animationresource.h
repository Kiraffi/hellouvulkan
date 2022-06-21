#pragma once

enum class PlayMode
{
    PlayOnce,
    Loop
};

struct AnimationResource
{
    float length;
    PlayMode playmode;
};