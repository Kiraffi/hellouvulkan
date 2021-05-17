#pragma once

#include <core/mytypes.h>

#include <vector>
#include <cstring> //memcmp
#include <glm/glm.hpp> // min max
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
const u32 trianglesPerPatch = 32u;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;

	bool operator==(const Vertex& other) const
	{
		return memcmp(this, &other, sizeof(Vertex)) == 0;
	}
};



struct Meshlet
{
	glm::vec3 normal;
	float normalAngleMax;

	glm::vec3 pos;
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
	BoundingBox(glm::vec3 firstPoint)
	{
		min = max = firstPoint;
	}

	void addPoint(glm::vec3 point)
	{
		min = glm::min(min, point);
		max = glm::max(max, point);
	}
	glm::vec3 min;
	glm::vec3 max;
};


bool loadMesh(Mesh &result, const char *path, size_t startIndex);