#pragma once

constexpr double Pi = 3.14159265358979323846264; //constant expression

static inline float toRadians(float angle)
{
    return float((angle / 180.0f) * Pi);
}
static inline float toDegrees(float angle)
{
    return float(180.0f / Pi * angle);
}
