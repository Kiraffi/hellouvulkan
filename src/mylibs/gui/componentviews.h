#pragma once
struct Transform;
struct GameEntity;

struct Vector3;
struct Quaternion;

void drawTransform(Transform &transform);
bool drawEntity(const char *windowStr, GameEntity &entity);
bool drawEntityContents(GameEntity &entity);


void drawVec3(const char *name, Vector3 &v);
void drawQuat(const char *name, Quaternion &q);

