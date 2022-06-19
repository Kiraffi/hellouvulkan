#pragma once
struct Transform;
struct GameEntity;

struct Vector3;
struct Quaternion;

void drawTransform(Transform &transform);

void drawVec3(const char *name, Vector3 &v);
void drawQuat(const char *name, Quaternion &q);

