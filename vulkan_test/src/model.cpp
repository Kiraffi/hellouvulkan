
#include "model.h"


#include <algorithm>
#include <cmath>
#include <core/mytypes.h>
#include <core/timer.h>

#include <stb/stb_image.h>

#include <tiny_obj_loader.h>
#include <meshoptimizer/src/meshoptimizer.h>


/*
// For hashmap
namespace std 
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			Vec3 pos(vertex.pos);
			Vec3 norm(vertex.normal);
			Vec2 tc(vertex.texCoord);
			return ((hash<Vec3>()(pos) ^ (hash<Vec3>()(norm) << 1)) >> 1) ^ (hash<Vec2>()(tc) << 1);
		}
	};
}
*/







static void buildMeshlets(Mesh &mesh)
{
	ScopedTimer scopedTimer("Build mesh cull data");

	std::vector<Meshlet> meshlets((mesh.indices.size() + 3 * trianglesPerPatch - 1) / (3 * trianglesPerPatch));

	u32 meshletIndex = 0u;

	u32 kulmain = 0u;

	for(u32 i = 0; i < u32(mesh.indices.size()); i += 3 * trianglesPerPatch)
	{
		u32 maxAmount = std::min((u32(mesh.indices.size()) - i) / 3, trianglesPerPatch);
		if(maxAmount < 3)
			continue;
		Vec3 dirs[trianglesPerPatch];


		BoundingBox bbox(mesh.vertices[mesh.indices[i]].pos);

		for(u32 j = 0; j < maxAmount; ++j)
		{
			u32 ind0 = mesh.indices[i + j * 3 + 0];
			u32 ind1 = mesh.indices[i + j * 3 + 1];
			u32 ind2 = mesh.indices[i + j * 3 + 2];


			bbox.addPoint(mesh.vertices[ind0].pos);
			bbox.addPoint(mesh.vertices[ind1].pos);
			bbox.addPoint(mesh.vertices[ind2].pos);

			Vec3 v1 = normalize(mesh.vertices[ind1].pos - mesh.vertices[ind0].pos); 
			Vec3 v2 = normalize(mesh.vertices[ind2].pos - mesh.vertices[ind0].pos); 
			dirs[j] = normalize(cross(v1, v2));
		}

		
		Vec3 normal = dirs[0];

		// 1.0f same direction
		float biggestAngleCos = 1000.0f;
		for(u32 j = 0; j < maxAmount; ++j)
		{
			for(u32 k = j + 1; k < maxAmount; ++k)
			{
				float newDot = dot(dirs[j], dirs[k]);
				if(newDot < biggestAngleCos)
				{
					biggestAngleCos = newDot;
					normal = normalize(dirs[j] + dirs[k]);
				}
			}
		}
		float maxAngle = -100.0f;
		float minAngleRadius = 100.0f;
		for(u32 j = 0; j < maxAmount; ++j)
		{
			float d = dot(dirs[j], normal);
			maxAngle = std::max(maxAngle, d);
			minAngleRadius = std::min(minAngleRadius, d);
		}

		Meshlet &meshlet = meshlets[meshletIndex];
		meshlet.normal = normal;
		meshlet.normalAngleMax = std::acos(minAngleRadius);
//		meshlet.normalAngleMax = minAngleRadius; // std::acos(minAngleRadius);
		meshlet.pos = (bbox.min + bbox.max) * 0.5f;
		meshlet.radius = (float)(len(((bbox.max - bbox.min) * 0.5f)));
	
		float ang = std::acos(minAngleRadius);
		if(ang / (2.0f * 3.14159f) *360.0f > 25.0f)
			++kulmain;
		
			//printf("angle: %f\n", ang / 3.14196f * 180.0f );
		++meshletIndex;
	}

	printf("yli: %u / %u\n", kulmain, meshletIndex);

	mesh.meshlets = meshlets;
}


struct Triangle
{
	std::vector<u32> connectedTriangles;
	u32 indices[3];
	Vec3 normal;
	Vec3 midPoint;
};


struct Candidates
{
	u32 index;
	float dotNormal;

	static bool sorter(const Candidates &a, const Candidates &b)
	{
		return a.index > b.index;
	}
};

void mergesort(Candidates* data, u32* sorted, u32 start, u32 end)
{
	if(start + 1 == end)
		return;
	u32 m = (start + end) / 2;
	mergesort(data, sorted, start, m);
	mergesort(data, sorted, m, end);

	u32 index = start;

	while(start < m && m < end)
	{
		if(data[start].index > data[m].index)
			sorted[index++] = data[start++].index;
		else
			sorted[index++] = data[m++].index;
	}
	while(start < m)
		sorted[index++] = data[start++].index;
	while(m < end)
		sorted[index++] = data[start++].index;
}

const u32 tBatchSize = 16u;

void optimizeMeshGeometry(Mesh &mesh)
{
	ScopedTimer scopedTimer("So called optimizing");
	std::vector<Triangle> triangles(mesh.indices.size() / 3);

	for(u32 i = 0; i < triangles.size(); ++i)
	{
		Triangle &triangle = triangles[i];
		u32 inds[3] = {};
		for(u32 j = 0; j < 3; ++j)
			triangle.indices[j] = inds[j] = mesh.indices[i * 3 + j];

		Vec3 v1 = normalize(mesh.vertices[inds[1]].pos - mesh.vertices[inds[0]].pos); 
		Vec3 v2 = normalize(mesh.vertices[inds[2]].pos - mesh.vertices[inds[0]].pos); 
		BoundingBox bb(mesh.vertices[inds[0]].pos);
		bb.addPoint(mesh.vertices[inds[1]].pos);
		bb.addPoint(mesh.vertices[inds[2]].pos);
		triangle.midPoint = (bb.max + bb.min) * 0.5f;

		triangle.normal = normalize(cross(v1, v2));
	}

	std::vector<u32> newIndices;//(mesh.indices.size());

	//u32 dInd = 0u;
	//u32 sortedTriangles[tBatchSize];
	while(triangles.size() > 0)
	{
		std::vector<Candidates> dotNormals;
		Vec3 normal = triangles[0].normal;
		Vec3 mp = triangles[0].midPoint;
		for(u32 j = 1; j < triangles.size(); ++j)
		{
			float dotNormal = dot(normal, triangles[j].normal) / (std::max(1.0e-12f, float(len(triangles[j].midPoint - mp))));
			if(dotNormals.size() < tBatchSize - 1 || dotNormals[tBatchSize - 1].dotNormal < dotNormal)
			{
				std::vector<Candidates>::iterator iter = dotNormals.begin();
				Candidates add {j, dotNormal};
				bool added = false;
				while(iter < dotNormals.end())
				{
					if(iter->dotNormal < dotNormal)
					{
						dotNormals.insert(iter, add);
						added = true;
						break;
					}
					iter++;
				}
				if(!added)
					dotNormals.insert(iter, add);

				if(dotNormals.size() >= tBatchSize)
					dotNormals.resize(tBatchSize - 1);

			}
		}

		dotNormals.emplace_back(Candidates{0, 0.0f});
		for(u32 i = 0; i < dotNormals.size(); ++i)
		{
			u32 *p = triangles[dotNormals[i].index].indices;
			for(u32 k = 0; k < 3; ++k)
			{
				newIndices.push_back(*p++);
			}
		}
		

		std::sort(dotNormals.begin(), dotNormals.end(), Candidates::sorter);
		for(u32 i = 0; i < dotNormals.size(); ++i)
		{
			u32 index = dotNormals[i].index;			
			triangles[index] = triangles[triangles.size() - i - 1];
		}
		triangles.resize(triangles.size() - dotNormals.size());
	}
	mesh.indices = newIndices;
}


#if 0
bool loadMesh(Mesh &result, const char *path, size_t startIndex)
{
	{
		ScopedTimer scopedTimer("Load mesh");

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path))
			return false;
		std::vector<Vertex> vertices;

		std::vector<u32> indices;

		std::unordered_map<Vertex, u32> uniqueVertices;

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex = {};

				ASSERT(index.vertex_index >= 0);
				vertex.pos.x = attrib.vertices[3 * index.vertex_index + 0];
				vertex.pos.y = attrib.vertices[3 * index.vertex_index + 1];
				vertex.pos.z = attrib.vertices[3 * index.vertex_index + 2];

				if(index.texcoord_index > 0)
				{
					vertex.texCoord.x = attrib.texcoords[2 * index.texcoord_index + 0];
					vertex.texCoord.y = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
				}
				if(index.normal_index > 0)
				{
					vertex.normal.x = attrib.normals[3 * index.normal_index + 0];
					vertex.normal.y = attrib.normals[3 * index.normal_index + 1];
					vertex.normal.z = attrib.normals[3 * index.normal_index + 2];
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}

/*
		u32 indexCount = u32(vertices.size());



		ASSERT(indexCount % 48 == 0);

		std::vector<u32> remap(indexCount);
		size_t vertexCount = meshopt_generateVertexRemap(remap.data(), 0, indexCount, vertices.data(), indexCount, sizeof(Vertex));

		std::vector<Vertex> optimizedVertices(vertexCount);
		std::vector<u32> indices(indexCount);

		meshopt_remapVertexBuffer(optimizedVertices.data(), vertices.data(), indexCount, sizeof(Vertex), remap.data());
		meshopt_remapIndexBuffer(indices.data(), 0, indexCount, remap.data());

		meshopt_optimizeVertexCache(indices.data(), indices.data(), indexCount, vertexCount);
		meshopt_optimizeVertexFetch(optimizedVertices.data(), indices.data(), indexCount, optimizedVertices.data(), vertexCount, sizeof(Vertex));
*/
		result.vertices = vertices;
		result.indices = indices;
		printf("vertex count: %u, index count: %u\n", vertices.size(), indices.size());
	}

	optimizeMeshGeometry(result);

	buildMeshlets(result);

	for (u32 i = 0; i < (u32)result.indices.size(); ++i)
		result.indices[i] += (u32)startIndex;

	return true;

}


#else
bool loadMesh(Mesh &result, const char *path, size_t startIndex)
{
	{
		ScopedTimer scopedTimer("Load mesh");

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path))
			return false;
		std::vector<Vertex> vertices;

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex = {};

				ASSERT(index.vertex_index >= 0);
				vertex.pos.x = attrib.vertices[3 * index.vertex_index + 0];
				vertex.pos.y = attrib.vertices[3 * index.vertex_index + 1];
				vertex.pos.z = attrib.vertices[3 * index.vertex_index + 2];

				if(index.texcoord_index > 0)
				{
					vertex.texCoord.x = attrib.texcoords[2 * index.texcoord_index + 0];
					vertex.texCoord.y = 1.0f - attrib.texcoords[2 * index.texcoord_index + 1];
				}
				if(index.normal_index > 0)
				{
					vertex.normal.x = attrib.normals[3 * index.normal_index + 0];
					vertex.normal.y = attrib.normals[3 * index.normal_index + 1];
					vertex.normal.z = attrib.normals[3 * index.normal_index + 2];
				}
				vertices.push_back(vertex);
			}
		}

		u32 indexCount = u32(vertices.size());



		ASSERT(indexCount % 48 == 0);

		std::vector<u32> remap(indexCount);
		size_t vertexCount = meshopt_generateVertexRemap(remap.data(), 0, indexCount, vertices.data(), indexCount, sizeof(Vertex));

		std::vector<Vertex> optimizedVertices(vertexCount);
		std::vector<u32> indices(indexCount);

		meshopt_remapVertexBuffer(optimizedVertices.data(), vertices.data(), indexCount, sizeof(Vertex), remap.data());
		meshopt_remapIndexBuffer(indices.data(), 0, indexCount, remap.data());

		meshopt_optimizeVertexCache(indices.data(), indices.data(), indexCount, vertexCount);
		meshopt_optimizeVertexFetch(optimizedVertices.data(), indices.data(), indexCount, optimizedVertices.data(), vertexCount, sizeof(Vertex));

		result.vertices = optimizedVertices;
		result.indices = indices;
	}



	buildMeshlets(result);


	
	for (u32 i = 0; i < (u32)result.indices.size(); ++i)
		result.indices[i] += (u32)startIndex;

	return true;
}
#endif






