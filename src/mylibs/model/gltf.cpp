#include "gltf.h"

#include <core/general.h>
#include <core/json.h>

#include <math/matrix.h>
#include <math/quaternion.h>

bool readGLTF(const char *filename, RenderModel &outModel)
{
    std::string fName = std::string(filename);
    PodVector<char> buffer;

    if(!loadBytes(fName, buffer))
        return false;

    JSONBlock bl;
    bool parseSuccess = bl.parseJSON(ArraySliceView(buffer.data(), buffer.size()));
    printf("parsed: %i\n", parseSuccess);
    if(!parseSuccess)
        return false;

    if(parseSuccess)
    {
//        bl.print();
    }


    struct SceneNode
    {
        std::string name;
        Quat rot;
        Vec3 trans;
        uint32_t meshIndex = ~0u;
        uint32_t skinIndex = ~0u;
        PodVector<uint32_t> childNodeIndices;
    };

    struct MeshNode
    {
        std::string name;
        uint32_t positionIndex = ~0u;
        uint32_t normalIndex = ~0u;
        uint32_t uvIndex = ~0u;
        uint32_t colorIndex = ~0u;
        uint32_t indicesIndex = ~0u;
        uint32_t materialIndex = ~0u;
        uint32_t jointsIndex = ~0u;
        uint32_t weightIndex = ~0u;
    };

    struct SkinNode
    {
        std::string name;
        PodVector<uint32_t> joints;
        uint32_t inverseMatricesIndex = ~0u;

        PodVector<Matrix> inverseMatrices;
    };

    std::vector<SceneNode> nodes;
    std::vector<MeshNode> meshes;
    std::vector<SkinNode> skins;
    std::vector<PodVector<uint8_t>> buffers;


    struct AnimationData
    {

    };

    if(!bl.isObject() || bl.getChildCount() < 1)
        return false;

    {
        const JSONBlock &meshBlock = bl.getChild("meshes");
        if(!meshBlock.isValid() || meshBlock.getChildCount() < 1)
            return false;

        meshes.resize(meshBlock.getChildCount());

        for(int i = 0; i < meshBlock.getChildCount(); ++i)
        {
            const JSONBlock &child = meshBlock.children [i];
            MeshNode &node = meshes [i];
            if(!child.getChild("name").parseString(node.name))
                return false;

            const JSONBlock &prims = child.getChild("primitives").getChild(0);
            if(!prims.isValid())
                return false;

            if(!prims.getChild("indices").parseUInt(node.indicesIndex) ||
                !prims.getChild("material").parseUInt(node.materialIndex))
                return false;

            const JSONBlock &attribs = prims.getChild("attributes");
            if(!attribs.isValid() || attribs.getChildCount() < 1)
                return false;

            if(!attribs.getChild("POSITION").parseUInt(node.positionIndex) ||
                !attribs.getChild("NORMAL").parseUInt(node.normalIndex) ||
                !attribs.getChild("COLOR_0").parseUInt(node.colorIndex))
                return false;

            attribs.getChild("TEXCOORD_0").parseUInt(node.uvIndex);

            attribs.getChild("JOINTS_0").parseUInt(node.jointsIndex);
            attribs.getChild("WEIGHTS_0").parseUInt(node.weightIndex);


        }
    }
    {
        const JSONBlock &nodeBlock = bl.getChild("nodes");
        if(!nodeBlock.isValid() || nodeBlock.getChildCount() < 1)
            return false;

        nodes.resize(nodeBlock.getChildCount());

        for(int i = 0; i < nodeBlock.getChildCount(); ++i)
        {
            const JSONBlock &child = nodeBlock.children [i];
            SceneNode &node = nodes [i];
            if(!child.getChild("name").parseString(node.name))
                return false;

            child.getChild("mesh").parseUInt(node.meshIndex);
            child.getChild("skin").parseUInt(node.skinIndex);


            const JSONBlock &rotBlock = child.getChild("rotation");
            rotBlock.getChild(0).parseNumber(node.rot.v.x);
            rotBlock.getChild(1).parseNumber(node.rot.v.y);
            rotBlock.getChild(2).parseNumber(node.rot.v.z);
            rotBlock.getChild(3).parseNumber(node.rot.w);

            const JSONBlock &transBlock = child.getChild("translation");
            transBlock.getChild(0).parseNumber(node.trans.x);
            transBlock.getChild(1).parseNumber(node.trans.y);
            transBlock.getChild(2).parseNumber(node.trans.z);


            const JSONBlock& childrenBlock = child.getChild("children");
            for (int childIndex = 0; childIndex < childrenBlock.getChildCount(); ++childIndex)
            {
                uint32_t tmpIndex = ~0u;
                if (!childrenBlock.getChild(childIndex).parseUInt(tmpIndex))
                    return false;
                node.childNodeIndices.push_back(tmpIndex);
            }

        }
    }

    {
        const JSONBlock& skinBlock = bl.getChild("skins");
        if (skinBlock.isValid() && skinBlock.getChildCount() > 0)
        {
            skins.resize(skinBlock.getChildCount());

            for (int i = 0; i < skinBlock.getChildCount(); ++i)
            {
                SkinNode& node = skins[i];
                const JSONBlock& child = skinBlock.children[i];

                if (!child.getChild("name").parseString(node.name))
                    return false;

                if (!child.getChild("inverseBindMatrices").parseUInt(node.inverseMatricesIndex))
                    return false;

                const JSONBlock& jointsBlock = child.getChild("joints");
                if (!jointsBlock.isValid() || jointsBlock.getChildCount() < 1)
                    return false;

                for (int j = 0; j < jointsBlock.getChildCount(); ++j)
                {
                    uint32_t tmpInt = ~0u;
                    if (!jointsBlock.getChild(j).parseUInt(tmpInt))
                        return false;

                    node.joints.push_back(tmpInt);
                }

            }
        }
    }

     {
        const JSONBlock &bufferBlock = bl.getChild("buffers");
        if(!bufferBlock.isValid() || bufferBlock.getChildCount() < 1)
            return false;

        buffers.resize(bufferBlock.getChildCount());

        for(int i = 0; i < bufferBlock.getChildCount(); ++i)
        {
            const JSONBlock &child = bufferBlock.children [i];
            PodVector<uint8_t> &buffer = buffers [i];

            uint32_t bufLen = 0u;
            if(!child.getChild("byteLength").parseUInt(bufLen))
                return false;

            if(!child.getChild("uri").parseBuffer(buffer))
                return false;

            if(bufLen != buffer.size())
                return false;
        }
    }

    {
        const JSONBlock &accessorBlock = bl.getChild("accessors");
        if(!accessorBlock.isValid() || accessorBlock.getChildCount() < 1)
            return false;

        const JSONBlock &bufferViewBlock = bl.getChild("bufferViews");
        if(!bufferViewBlock.isValid() || bufferViewBlock.getChildCount() < 1)
            return false;


        MeshNode &node = meshes [0];

        auto lam =[&](uint32_t index, int32_t floatStartOffsetIndex, bool useVertices)
        {
            if(index == ~0u || index >= accessorBlock.getChildCount())
                return false;

            const JSONBlock &block = accessorBlock.getChild(index);
            if(!block.isValid())
                return false;

            uint32_t viewIndex = ~0u;
            uint32_t componentType = ~0u;
            uint32_t count = ~0u;
            bool normalized = false;
            std::string s;

            if(!block.getChild("bufferView").parseUInt(viewIndex)
             || !block.getChild("componentType").parseUInt(componentType)
             || !block.getChild("count").parseUInt(count)
             || !block.getChild("type").parseString(s)
            )
                return false;

            if (block.getChild("sparse").isValid())
            {
                LOG("No sparse view are handled!\n");
                ASSERT(false && "No sparse view are handled");
                return false;
            }


            block.getChild("normalized").parseBool(normalized);

            uint32_t componentCount = ~0u;

            //"SCALAR"     1
            //"VEC2"     2
            //"VEC3"     3
            //"VEC4"     4
            //"MAT2"     4
            //"MAT3"     9
            //"MAT4"     16

            if (s == "SCALAR") componentCount = 1;
            else if (s == "VEC2") componentCount = 2;
            else if (s == "VEC3") componentCount = 3;
            else if (s == "VEC4") componentCount = 4;
            else if (s == "MAT2") componentCount = 4;
            else if (s == "MAT3") componentCount = 9;
            else if (s == "MAT4") componentCount = 16;
            else return false;

            //// Maybe 5124 is half?
            //5120 (BYTE)1
            //5121(UNSIGNED_BYTE)1
            //5122 (SHORT)2
            //5123 (UNSIGNED_SHORT)2
            //5125 (UNSIGNED_INT)4
            //5126 (FLOAT)4

            uint32_t componentTypeBitCount = 0u;
            switch(componentType)
            {
            case 5120:
            case 5121:
                componentTypeBitCount = 1u;
                break;

            case 5122:
            case 5123:
                componentTypeBitCount = 2u;
                break;

            case 5125:
            case 5126:
                componentTypeBitCount = 4u;
                break;

            default:
                return false;
            }
            uint32_t bufferIndex = ~0u;
            uint32_t bufferOffset = ~0u;
            uint32_t bufferLen = ~0u;

            const JSONBlock &bufferBlock = bufferViewBlock.getChild(viewIndex);
            if(!bufferBlock.isValid())
                return false;

            if(!bufferBlock.getChild("buffer").parseUInt(bufferIndex)
                || !bufferBlock.getChild("byteLength").parseUInt(bufferLen)
                || !bufferBlock.getChild("byteOffset").parseUInt(bufferOffset)
                )
                return false;

            if(bufferIndex >= buffers.size() || bufferOffset + bufferLen > buffers[bufferIndex].size() )
                return false;

            uint8_t *ptr = &buffers[bufferIndex][0] + bufferOffset;
            uint8_t *endPtr = &buffers[bufferIndex][0] + bufferOffset + bufferLen;


            // Doesnt exactly handle cases properly... Just reading stuff into float buffer, in case its either normalized u16 value or 32 bit float.
            if(useVertices)
            {
                bool isValidVertice = componentType == 5126 || (componentType == 5123 && normalized) || (componentType == 5121);
                ASSERT(isValidVertice);
                if (!isValidVertice)
                    return false;


                if(outModel.vertices.size() == 0)
                    outModel.vertices.resize(count);
                if(outModel.vertices.size() != count)
                    return false;

                for(uint32_t i = 0; i < count; ++i)
                {

                    RenderModel::Vertex &v = outModel.vertices[i];
                    float *f = (float *)(((uint8_t *)&v) + floatStartOffsetIndex);
                    for(uint32_t j = 0; j < componentCount; ++j)
                    {
                        if(ptr + componentTypeBitCount > endPtr)
                            return false;

                        float f1 = 0.0f;
                        uint32_t u1 = 0u;


                        if (componentType == 5126)
                        {
                            memcpy(&f1, ptr, componentTypeBitCount);
                        }
                        else if(componentType == 5123 && normalized)
                        {
                            uint16_t tmp = 0;
                            memcpy(&tmp, ptr, componentTypeBitCount);

                            f1 = (float)tmp / 65535.0f;
                        }
                        else
                        {
                            return false;
                        }


                        *(f + j) = f1;
                        ptr += componentTypeBitCount;
                    }
                }
            }
            // Assumption that all indices are either u16 or u32 values.
            else
            {
                bool isValidIndice = componentType == 5123 || componentType == 5125;
                ASSERT(isValidIndice);
                if (!isValidIndice)
                    return false;


                if(outModel.indices.size() != 0)
                    return false;
                outModel.indices.resize(count);

                for(uint32_t i = 0; i < count; ++i)
                {
                    if(ptr + componentTypeBitCount > endPtr)
                        return false;

                    uint32_t value = 0u;
                    if(componentTypeBitCount == 4)
                        memcpy(&value, ptr, componentTypeBitCount);
                    else if(componentTypeBitCount == 2)
                    {
                        uint16_t tmp = 0;
                        memcpy(&tmp, ptr, componentTypeBitCount);

                        value = tmp;
                    }
                    else
                        return false;

                    outModel.indices[i] = value;

                    ptr += componentTypeBitCount;
                }
            }
            return true;

        };

        if(!lam(node.positionIndex, offsetof(RenderModel::Vertex, pos), true))
            return false;

        if(!lam(node.normalIndex, offsetof(RenderModel::Vertex, norm), true))
            return false;

        if(!lam(node.colorIndex, offsetof(RenderModel::Vertex, color), true))
            return false;

        if(!lam(node.indicesIndex, 0, false))
            return false;
    }

/*
    for(uint32_t i = 0; i < vertices.size(); ++i)
    {
        printf("i: %i:   x: %f, y: %f, z: %f\n", i, vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z);
        printf("i: %i:   x: %f, y: %f, z: %f\n", i, vertices[i].norm.x, vertices[i].norm.y, vertices[i].norm.z);
        printf("i: %i:   x: %f, y: %f, z: %f, w: %f\n", i, vertices[i].color.x, vertices[i].color.y, vertices[i].color.z, vertices[i].color.w);
    }

    for(uint32_t i = 0; i < indices.size(); ++i)
    {
        printf("i: %i, index: %u\n", i, indices[i]);
    }
*/
    return true;
}
