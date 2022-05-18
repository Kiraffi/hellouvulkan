#include "gltf.h"

#include <core/general.h>
#include <core/json.h>

#include <math/matrix.h>
#include <math/quaternion.h>

struct GltfMemoryRange
{
    uint8_t *start = nullptr;
    uint8_t *end = nullptr;
};

template<typename T>
GltfMemoryRange getMemoryRange(const std::vector<T> &vec)
{
    return { .start = ( uint8_t * )( &*vec.begin() ), .end = ( uint8_t * )( &*vec.begin() + vec.size() ) };
}

//Type      component count
//"SCALAR"     1
//"VEC2"     2
//"VEC3"     3
//"VEC4"     4
//"MAT2"     4
//"MAT3"     9
//"MAT4"     16
enum class GltfBufferComponentCountType
{
    NONE,
    SCALAR,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4,
};

//// Maybe 5124 is int?
//5120 (BYTE)1
//5121(UNSIGNED_BYTE)1
//5122 (SHORT)2
//5123 (UNSIGNED_SHORT)2
//5125 (UNSIGNED_INT)4
//5126 (FLOAT)4
enum class GltfBufferComponentType
{
    NONE,
    INT_8,
    INT_16,
    INT_32,
    UINT_8,
    UINT_16,
    UINT_32,
    FLOAT_16,
    FLOAT_32,
};

struct GltfSceneNode
{
    std::string name;
    Quat rot;
    Vec3 trans;
    uint32_t meshIndex = ~0u;
    uint32_t skinIndex = ~0u;
    std::vector<uint32_t> childNodeIndices;
};

struct GltfMeshNode
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

struct GltfSkinNode
{
    std::string name;
    std::vector<uint32_t> joints;
    uint32_t inverseMatricesIndex = ~0u;

    std::vector<Matrix> inverseMatrices;
};

struct GltfAnimationNode
{
    struct Channel
    {
        enum class ChannelPath : uint8_t
        {
            NONE,
            TRANSLATION,
            ROTAION,
            SCALE,
        };
        uint32_t samplerIndex = ~0u;
        uint32_t nodeIndex = ~0u;
        ChannelPath pathType = ChannelPath::NONE;
    };
    struct Sampler
    {
        enum class Interpolation : uint8_t
        {
            NONE,
            LINEAR,
        };
        uint32_t inputIndex = ~0u;
        uint32_t outputIndex = ~0u;
        Interpolation interpolationType = Interpolation::NONE;

    };
    std::vector<Channel> channels;
    std::vector<Sampler> samplers;
    std::string name;
};

struct GltfBufferView
{
    uint32_t bufferIndex = ~0u;
    uint32_t bufferStartOffset = 0u;
    uint32_t bufferLength = 0u;
};

struct GltfBufferAccessor
{
    uint32_t bufferViewIndex = ~0u;
    uint32_t count = 0u;
    GltfBufferComponentCountType countType;
    GltfBufferComponentType componentType;
    std::vector<double> mins;
    std::vector<double> maxs;
    bool normalized = false;
    bool useMinMax = false;
};

struct GltfData
{
    std::vector<GltfSceneNode> nodes;
    std::vector<GltfMeshNode> meshes;
    std::vector<GltfSkinNode> skins;
    std::vector<GltfAnimationNode> animationNodes;

    std::vector<GltfBufferAccessor> accessors;
    std::vector<GltfBufferView> bufferViews;

    std::vector<PodVector<uint8_t>> buffers;
};







// if buffer + offsetInStruct + stride * indexcount can write past buffer......
// Reads f32, float normalized u16 and normalized u8 into float,
// Reads u32, from u8, u16, u32
// doesnt handle i8, i16 nor i32 reading, should probably just read them into i32.
static bool gltfReadIntoBuffer(GltfData &data, uint32_t index,
        uint32_t writeStartOffsetInBytes, uint32_t writeStrideInBytes,
        GltfMemoryRange memoryRange)
{
    if(index >= data.accessors.size())
        return false;

    const GltfBufferAccessor &accessor = data.accessors [index];
    if(accessor.bufferViewIndex >= data.bufferViews.size())
        return false;

    const GltfBufferView &bufferView = data.bufferViews [accessor.bufferViewIndex];
    if(bufferView.bufferIndex >= data.buffers.size())
        return false;

    uint8_t *ptr = &data.buffers [bufferView.bufferIndex] [0] + bufferView.bufferStartOffset;
    const uint8_t *endPtr = ptr + bufferView.bufferLength;

    uint32_t componentCount = ~0u;

    //"SCALAR"     1
    //"VEC2"     2
    //"VEC3"     3
    //"VEC4"     4
    //"MAT2"     4
    //"MAT3"     9
    //"MAT4"     16
    switch(accessor.countType)
    {
        case GltfBufferComponentCountType::SCALAR: componentCount = 1; break;
        case GltfBufferComponentCountType::VEC2: componentCount = 2; break;
        case GltfBufferComponentCountType::VEC3: componentCount = 3; break;
        case GltfBufferComponentCountType::VEC4: componentCount = 4; break;
        case GltfBufferComponentCountType::MAT2: componentCount = 4; break;
        case GltfBufferComponentCountType::MAT3: componentCount = 9; break;
        case GltfBufferComponentCountType::MAT4: componentCount = 16; break;
        case GltfBufferComponentCountType::NONE: return false;
    }
    uint32_t componentTypeBitCount = ~0u;
    switch(accessor.componentType)
    {
        case GltfBufferComponentType::FLOAT_32:
        case GltfBufferComponentType::INT_32:
        case GltfBufferComponentType::UINT_32:
            componentTypeBitCount = 4u;
            break;
        case GltfBufferComponentType::FLOAT_16:
        case GltfBufferComponentType::INT_16:
        case GltfBufferComponentType::UINT_16:
            componentTypeBitCount = 2u;
            break;
        case GltfBufferComponentType::INT_8:
        case GltfBufferComponentType::UINT_8:
            componentTypeBitCount = 1u;
            break;
        case GltfBufferComponentType::NONE:
            return false;
    }

    uint8_t *bufferOut = memoryRange.start + writeStartOffsetInBytes;
    if(accessor.count == 0u)
        return false;

    // Since we write either float or uint32_t size of them is 4, check if we would write out of
    // buffer.
    uint8_t *countedLast = bufferOut + ( accessor.count - 1 ) * writeStrideInBytes +
        sizeof(uint32_t) * componentCount;
    if(countedLast > memoryRange.end)
        return false;

    // Reading f32, normalized u16 and normalized u8 as float.
    if(accessor.componentType == GltfBufferComponentType::FLOAT_32 ||
        ( accessor.componentType == GltfBufferComponentType::UINT_16 && accessor.normalized ) ||
        ( accessor.componentType == GltfBufferComponentType::UINT_8 && accessor.normalized ))
    {
        for(uint32_t i = 0; i < accessor.count; ++i)
        {
            float *writeValue = ( float * )bufferOut;
            for(uint32_t j = 0; j < componentCount; ++j)
            {
                if(ptr + componentTypeBitCount > endPtr)
                {
                    ASSERT(false && "Trying to read past buffer end");
                    return false;
                }
                // float* ptr + 1 adds 4
                if(writeValue + 1 > ( void * )memoryRange.end)
                {
                    ASSERT(false && "Trying to write past writeBufferEnd");
                    return false;
                }

                if(accessor.componentType == GltfBufferComponentType::FLOAT_32)
                {
                    memcpy(writeValue, ptr, componentTypeBitCount);
                }
                else if(accessor.componentType == GltfBufferComponentType::UINT_16 && accessor.normalized)
                {
                    uint16_t tmp = 0;
                    memcpy(&tmp, ptr, componentTypeBitCount);
                    *writeValue = ( float )tmp / 65535.0f;
                }
                else if(accessor.componentType == GltfBufferComponentType::UINT_8 && accessor.normalized)
                {
                    uint8_t tmp = 0;
                    memcpy(&tmp, ptr, componentTypeBitCount);
                    *writeValue = ( float )tmp / 255.0f;
                }

                ++writeValue;
                ptr += componentTypeBitCount;
            }
            bufferOut += writeStrideInBytes;
        }
    }
    // i8, u8, i16, u16, i32, u32. as uint32_t
    else if(!accessor.normalized && (
        accessor.componentType == GltfBufferComponentType::UINT_8 ||
        accessor.componentType == GltfBufferComponentType::UINT_16 ||
        accessor.componentType == GltfBufferComponentType::UINT_32 ))
        //accessor.componentType == GltfBufferComponentType::INT_8 ||
        //accessor.componentType == GltfBufferComponentType::INT_16 ||
        //accessor.componentType == GltfBufferComponentType::INT_32 ))
    {
        for(uint32_t i = 0; i < accessor.count; ++i)
        {
            uint32_t *writeValue = ( uint32_t * )bufferOut;
            for(uint32_t j = 0; j < componentCount; ++j)
            {
                if(ptr + componentTypeBitCount > endPtr)
                    return false;
                // uint32_t* ptr + 1 adds 4
                if(writeValue + 1 > (void *)memoryRange.end)
                    return false;

                if(componentTypeBitCount == 4)
                {
                    memcpy(writeValue, ptr, componentTypeBitCount);
                }
                else if(componentTypeBitCount == 2)
                {
                    uint16_t tmp = 0;
                    memcpy(&tmp, ptr, componentTypeBitCount);
                    *writeValue = tmp;
                }
                else if(componentTypeBitCount == 1)
                {
                    uint8_t tmp = *ptr;
                    *writeValue = tmp;
                }

                ptr += componentTypeBitCount;
                ++writeValue;
            }
            bufferOut += writeStrideInBytes;
        }
    }
    else
    {
        // unsupported
        ASSERT(false && "Some type not supported");
        return false;
    }
    return true;

}






bool readGLTF(const char *filename, RenderModel &outModel)
{
    GltfData data;

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


    if(!bl.isObject() || bl.getChildCount() < 1)
        return false;


    // Parse meshes
    {
        const JSONBlock &meshBlock = bl.getChild("meshes");
        if(!meshBlock.isValid() || meshBlock.getChildCount() < 1)
            return false;

        data.meshes.resize(meshBlock.getChildCount());

        for(int i = 0; i < meshBlock.getChildCount(); ++i)
        {
            const JSONBlock &child = meshBlock.children [i];
            GltfMeshNode &node = data.meshes [i];
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

        data.nodes.resize(nodeBlock.getChildCount());

        for(int i = 0; i < nodeBlock.getChildCount(); ++i)
        {
            const JSONBlock &child = nodeBlock.children [i];
            GltfSceneNode &node = data.nodes [i];
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
            for(const auto &childBlock : childrenBlock.children)
            {
                uint32_t tmpIndex = ~0u;
                if(!childBlock.parseUInt(tmpIndex))
                    return false;
                node.childNodeIndices.push_back(tmpIndex);
            }
        }
    }

    {
        const JSONBlock& skinBlock = bl.getChild("skins");
        if (skinBlock.isValid() && skinBlock.getChildCount() > 0)
        {
            data.skins.resize(skinBlock.getChildCount());

            for (int i = 0; i < skinBlock.getChildCount(); ++i)
            {
                GltfSkinNode& node = data.skins[i];
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

        data.buffers.resize(bufferBlock.getChildCount());

        for(int i = 0; i < bufferBlock.getChildCount(); ++i)
        {
            const JSONBlock &child = bufferBlock.children [i];
            PodVector<uint8_t> &buffer = data.buffers [i];

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
        const JSONBlock &animationBlocks = bl.getChild("animations");
        if(animationBlocks.isValid() && animationBlocks.getChildCount() > 0)
        {
            data.animationNodes.resize(animationBlocks.getChildCount());
            for(uint32_t animIndex = 0u; animIndex < animationBlocks.getChildCount(); ++animIndex)
            {
                const JSONBlock &animationBlock = animationBlocks.getChild(animIndex);
                if(!animationBlock.isValid())
                    return false;
                GltfAnimationNode &animationNode = data.animationNodes[animIndex];
                if(!animationBlock.getChild("name").parseString(animationNode.name))
                    return false;
                const JSONBlock &channelsBlock = animationBlock.getChild("channels");
                const JSONBlock &samplersBlock = animationBlock.getChild("samplers");
                if(!channelsBlock.isValid() || !samplersBlock.isValid() ||
                    channelsBlock.getChildCount() == 0 || samplersBlock.getChildCount() == 0)
                    return false;

                animationNode.channels.resize(channelsBlock.getChildCount());
                animationNode.samplers.resize(samplersBlock.getChildCount());

                for(uint32_t i = 0; i < channelsBlock.getChildCount(); ++i)
                {
                    const JSONBlock &channelBlock = channelsBlock.getChild(i);
                    GltfAnimationNode::Channel &channel = animationNode.channels[i];
                    if(!channelBlock.getChild("sampler").parseUInt(channel.samplerIndex))
                        return false;
                    if(channel.samplerIndex >= animationNode.samplers.size())
                        return false;
                    if(!channelBlock.getChild("target").getChild("node").parseUInt(channel.nodeIndex))
                        return false;
                    std::string pathStr;
                    if(!channelBlock.getChild("target").getChild("path").parseString(pathStr))
                        return false;
                    if(pathStr == "translation")
                        channel.pathType = GltfAnimationNode::Channel::ChannelPath::TRANSLATION;
                    else if(pathStr == "rotation")
                        channel.pathType = GltfAnimationNode::Channel::ChannelPath::ROTAION;
                    else if(pathStr == "scale")
                        channel.pathType = GltfAnimationNode::Channel::ChannelPath::SCALE;
                    // unsupported?
                    else
                    {
                        ASSERT(false);
                        return false;
                    }
                }

                for(uint32_t i = 0; i < samplersBlock.getChildCount(); ++i)
                {
                    const JSONBlock &samplerBlock = samplersBlock.getChild(i);
                    GltfAnimationNode::Sampler &sampler = animationNode.samplers[i];
                    if(!samplerBlock.getChild("input").parseUInt(sampler.inputIndex))
                        return false;
                    if(!samplerBlock.getChild("output").parseUInt(sampler.outputIndex))
                        return false;

                    std::string interpolationStr;
                    if(!samplerBlock.getChild("interpolation").parseString(interpolationStr))
                        return false;
                    if(interpolationStr == "LINEAR")
                        sampler.interpolationType = GltfAnimationNode::Sampler::Interpolation::LINEAR;
                    // unsupported?
                    else
                    {
                        ASSERT(false);
                        return false;
                    }
                }
            }
        }
    }

    {
        const JSONBlock &accessorBlocks = bl.getChild("accessors");
        if(!accessorBlocks.isValid() || accessorBlocks.getChildCount() < 1)
            return false;

        const JSONBlock &bufferViewBlocks = bl.getChild("bufferViews");
        if(!bufferViewBlocks.isValid() || bufferViewBlocks.getChildCount() < 1)
            return false;

        data.accessors.resize(accessorBlocks.getChildCount());
        data.bufferViews.resize(bufferViewBlocks.getChildCount());

        for(uint32_t index = 0u; index < bufferViewBlocks.getChildCount(); ++index)
        {
            GltfBufferView &bufferView = data.bufferViews[index];
            const JSONBlock &bufferViewBlock = bufferViewBlocks.getChild(index);
            if(!bufferViewBlock.isValid())
                return false;

            if(!bufferViewBlock.isValid())
                return false;

            if(!bufferViewBlock.getChild("buffer").parseUInt(bufferView.bufferIndex)
                || !bufferViewBlock.getChild("byteLength").parseUInt(bufferView.bufferLength)
                || !bufferViewBlock.getChild("byteOffset").parseUInt(bufferView.bufferStartOffset)
                )
                return false;
        }


        for(uint32_t index = 0u; index < accessorBlocks.getChildCount(); ++index)
        {
            GltfBufferAccessor &accessor = data.accessors[index];
            const JSONBlock &accessorBlock = accessorBlocks.getChild(index);
            if(!accessorBlock.isValid())
                return false;

            uint32_t componentType = ~0u;
            std::string s;
            if(!accessorBlock.getChild("bufferView").parseUInt(accessor.bufferViewIndex)
                || !accessorBlock.getChild("componentType").parseUInt(componentType)
                || !accessorBlock.getChild("count").parseUInt(accessor.count)
                || !accessorBlock.getChild("type").parseString(s)
            )
                return false;

            if (accessorBlock.getChild("sparse").isValid())
            {
                LOG("No sparse view are handled!\n");
                ASSERT(false && "No sparse view are handled");
                return false;
            }
            if (accessorBlock.getChild("min").isValid())
            {
                accessor.mins.resize(accessorBlock.getChild("min").getChildCount());
                for(uint32_t minIndex = 0u; minIndex < accessor.mins.size(); ++minIndex)
                {
                    double value = 0.0;
                    if(!accessorBlock.getChild("min").getChild(minIndex).parseNumber(value))
                        return false;
                    accessor.mins[minIndex] = value;
                }
            }
            if (accessorBlock.getChild("max").isValid())
            {
                accessor.maxs.resize(accessorBlock.getChild("max").getChildCount());
                for(uint32_t maxIndex = 0u; maxIndex < accessor.maxs.size(); ++maxIndex)
                {
                    double value = 0.0;
                    if(!accessorBlock.getChild("max").getChild(maxIndex).parseNumber(value))
                        return false;
                    accessor.maxs[maxIndex] = value;
                }
            }

            if(!accessorBlock.getChild("normalized").parseBool(accessor.normalized))
                accessor.normalized = false;
            //"SCALAR"     1
            //"VEC2"     2
            //"VEC3"     3
            //"VEC4"     4
            //"MAT2"     4
            //"MAT3"     9
            //"MAT4"     16

            if (s == "SCALAR") accessor.countType = GltfBufferComponentCountType::SCALAR;
            else if (s == "VEC2") accessor.countType = GltfBufferComponentCountType::VEC2;
            else if (s == "VEC3") accessor.countType = GltfBufferComponentCountType::VEC3;
            else if (s == "VEC4") accessor.countType = GltfBufferComponentCountType::VEC4;
            else if (s == "MAT2") accessor.countType = GltfBufferComponentCountType::MAT2;
            else if (s == "MAT3") accessor.countType = GltfBufferComponentCountType::MAT3;
            else if (s == "MAT4") accessor.countType = GltfBufferComponentCountType::MAT4;
            else
            {
                ASSERT(false && "Unknown type for accessor!");
            }

            //// Maybe 5124 is int32?
            //5120 (BYTE)1
            //5121(UNSIGNED_BYTE)1
            //5122 (SHORT)2
            //5123 (UNSIGNED_SHORT)2
            //5125 (UNSIGNED_INT)4
            //5126 (FLOAT)4
            if(componentType == 5120) accessor.componentType = GltfBufferComponentType::INT_8;
            else if(componentType == 5121) accessor.componentType = GltfBufferComponentType::UINT_8;
            else if(componentType == 5122) accessor.componentType = GltfBufferComponentType::INT_16;
            else if(componentType == 5123) accessor.componentType = GltfBufferComponentType::UINT_16;
            else if(componentType == 5125) accessor.componentType = GltfBufferComponentType::UINT_32;
            else if(componentType == 5126) accessor.componentType = GltfBufferComponentType::FLOAT_32;
            else
            {
                ASSERT(false && componentType);
            }
        }
    }

    {

        GltfMeshNode &node = data.meshes [0];
        if(node.positionIndex >= data.accessors.size() ||
            node.normalIndex >= data.accessors.size() ||
            node.colorIndex >= data.accessors.size() ||

            node.indicesIndex >= data.accessors.size())
        {
            return false;
        }
        uint32_t vertexCount = data.accessors[node.positionIndex].count;
        if(vertexCount == ~0u || vertexCount == 0u ||
            vertexCount != data.accessors[node.normalIndex].count ||
            vertexCount != data.accessors[node.colorIndex].count)
        {
            return false;
        }
        uint32_t indicesCount = data.accessors[node.indicesIndex].count;
        if(indicesCount == ~0u || indicesCount == 0u)
            return false;

        // If only one of the jointsindex or weightindex exists.
        if((node.jointsIndex == ~0u) != (node.weightIndex == ~0u))
        {
            ASSERT(false && "Found either only jointindex or weightindex!");
        }

        if(data.accessors[node.positionIndex].countType != GltfBufferComponentCountType::VEC3)
            return false;
        if(data.accessors[node.normalIndex].countType != GltfBufferComponentCountType::VEC3)
            return false;
        if(data.accessors[node.colorIndex].countType != GltfBufferComponentCountType::VEC4)
            return false;

        if(data.accessors[node.indicesIndex].countType != GltfBufferComponentCountType::SCALAR)
            return false;

        outModel.vertices.resize(vertexCount);
        outModel.indices.resize(indicesCount);

        GltfMemoryRange verticesMemoryRange = getMemoryRange(outModel.vertices);
        if(!gltfReadIntoBuffer(data, node.positionIndex,
            offsetof(RenderModel::Vertex, pos), sizeof(RenderModel::Vertex),
            verticesMemoryRange))
            return false;

        if(!gltfReadIntoBuffer(data, node.normalIndex,
            offsetof(RenderModel::Vertex, norm), sizeof(RenderModel::Vertex),
            verticesMemoryRange))
            return false;

        if(!gltfReadIntoBuffer(data, node.colorIndex,
            offsetof(RenderModel::Vertex, color), sizeof(RenderModel::Vertex),
            verticesMemoryRange))
            return false;

        
        if(!gltfReadIntoBuffer(data, node.indicesIndex, 0, sizeof(uint32_t),
            getMemoryRange(outModel.indices)))
            return false;

        // Animation vertices?
        if(node.jointsIndex != ~0u && node.weightIndex != ~0u)
        {
            if(node.jointsIndex >= data.accessors.size() ||
                node.weightIndex >= data.accessors.size())
            {
                return false;
            }
            if(vertexCount != data.accessors[node.jointsIndex].count ||
                vertexCount != data.accessors[node.weightIndex].count)
            {
                return false;
            }
            outModel.animationVertices.resize(vertexCount);

            if(data.accessors[node.jointsIndex].countType != GltfBufferComponentCountType::VEC4)
                return false;
            if(data.accessors[node.weightIndex].countType != GltfBufferComponentCountType::VEC4)
                return false;

            if(!gltfReadIntoBuffer(data, node.weightIndex,
                offsetof(RenderModel::AnimationVertex, weights), sizeof(RenderModel::AnimationVertex),
                getMemoryRange(outModel.animationVertices)))
                return false;

            if(!gltfReadIntoBuffer(data, node.jointsIndex,
                offsetof(RenderModel::AnimationVertex, boneIndices), sizeof(RenderModel::AnimationVertex),
                getMemoryRange(outModel.animationVertices)))
                return false;

        }
        for(uint32_t i = 0; i < data.skins.size(); ++i)
        {
            GltfSkinNode &skinNode = data.skins[i];
            if(skinNode.inverseMatricesIndex >= data.accessors.size())
                return false;

            if(data.accessors[skinNode.inverseMatricesIndex].countType != GltfBufferComponentCountType::MAT4)
                return false;
            
            std::vector<Matrix> &matrices = skinNode.inverseMatrices;
            matrices.resize(data.accessors[skinNode.inverseMatricesIndex].count);

            if(!gltfReadIntoBuffer(data, skinNode.inverseMatricesIndex,
                0, sizeof(Matrix),
                getMemoryRange(matrices)))
                return false;

        }

        {
            std::vector<float> floats;
            floats.resize(data.accessors[8].count);
            if(data.accessors[8].countType != GltfBufferComponentCountType::SCALAR)
                return false;
            if(!gltfReadIntoBuffer(data, 8,
                0, sizeof(float),
                getMemoryRange(floats)))
                return false;
            for(float f: floats)
                printf("%.2f, ", f);
            printf("\n");
        }
    }

    const ArraySliceView<RenderModel::Vertex> vertices(&outModel.vertices[0], outModel.vertices.size());
    for(uint32_t i = 0; i < vertices.size(); ++i)
    {
        printf("vert: %i:   x: %f, y: %f, z: %f\n", i, vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z);
        printf("vert: %i:   x: %f, y: %f, z: %f\n", i, vertices[i].norm.x, vertices[i].norm.y, vertices[i].norm.z);
        printf("vert: %i:   x: %f, y: %f, z: %f, w: %f\n", i, vertices[i].color.x, vertices[i].color.y, vertices[i].color.z, vertices[i].color.w);
    }

    const ArraySliceView<RenderModel::AnimationVertex> animationVertices(
        &outModel.animationVertices[0], outModel.animationVertices.size());
    for(uint32_t i = 0; i < animationVertices.size(); ++i)
    {
        printf("vert: %i, bone0: %u, bone1: %u, bone2: %u, bone3: %u\n", i,
            animationVertices[i].boneIndices[0],
            animationVertices[i].boneIndices[1],
            animationVertices[i].boneIndices[2],
            animationVertices[i].boneIndices[3]);

        printf("vert: %i:   x: %f, y: %f, z: %f, w: %f\n", i,
            animationVertices[i].weights.x,
            animationVertices[i].weights.y,
            animationVertices[i].weights.z,
            animationVertices[i].weights.w);
    }

    const ArraySliceView<uint32_t> indices(&outModel.indices[0], outModel.indices.size());
    for(uint32_t i = 0; i < indices.size(); ++i)
    {
        printf("i: %i, index: %u\n", i, indices[i]);
    }

    for(uint32_t i = 0; i < data.skins.size(); ++i)
    {
        const GltfSkinNode &skin = data.skins[i];
        printf("Skin node: %u\n", i);
        for(uint32_t j = 0; j < skin.inverseMatrices.size(); ++j)
        {
            printf("Matrix: %i\n", j);
            printMatrix(skin.inverseMatrices[j], "");
        }
    }

    return true;
}
