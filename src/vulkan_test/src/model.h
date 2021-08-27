#pragma once

#include "core/mytypes.h"
#include "math/vector3.h"

#include <vector>

const u32 trianglesPerPatch = 32u;

struct Vertex
{
	Vec3 pos;
	Vec3 normal;
	Vec2 texCoord;

	bool operator==(const Vertex& other) const
	{
		return memcmp(this, &other, sizeof(Vertex)) == 0;
	}
};



struct Meshlet
{
	Vec3 normal;
	float normalAngleMax;

	Vec3 pos;
	float radius;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<u32> indices;
	std::vector<Meshlet> meshlets;
};



struct BoundingBox
{
	BoundingBox(Vec3 firstPoint)
	{
		min = max = firstPoint;
	}

	void addPoint(Vec3 point)
	{
		min.x = min.x < point.x ? min.x : point.x;
		min.y = min.y < point.y ? min.y : point.y;
		min.z = min.z < point.z ? min.z : point.z;

		max.x = max.x > point.x ? max.x : point.x;
		max.y = max.y > point.y ? max.y : point.y;
		max.z = max.z > point.z ? max.z : point.z;

	}
	Vec3 min;
	Vec3 max;
};


bool loadMesh(Mesh &result, const char *path, size_t startIndex);