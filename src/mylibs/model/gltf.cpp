#include "gltf.h"

#include <container/arraysliceview.h>
#include <container/podvector.h>
#include <container/podvectortypedefine.h>

#include <components/transform_functions.h>

#include <core/file.h>
#include <core/general.h>
#include <core/json.h>
#include <core/timer.h>

#include <math/matrix.h>
#include <math/quaternion_inline_functions.h>


struct GltfData;

static bool gltfReadIntoBuffer(const GltfData &data, u32 index,
        u32 writeStartOffsetInBytes, ArraySliceViewBytesMutable memoryRange);

static u32 findAccessorIndexAndParseTimeStamps(
    const GltfData &data, u32 samplerIndex,
    PodVector<float> &inOutTimestamps);


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
    StringView name;
    Quat rot;
    Vec3 trans;
    Vec3 scale{1.0f, 1.0f, 1.0f};
    u32 meshIndex = ~0u;
    u32 skinIndex = ~0u;
    PodVector<u32> childNodeIndices;
};

struct GltfMeshNode
{
    StringView name;
    u32 positionIndex = ~0u;
    u32 normalIndex = ~0u;
    u32 uvIndex = ~0u;
    u32 colorIndex = ~0u;
    u32 indicesIndex = ~0u;
    u32 materialIndex = ~0u;
    u32 jointsIndex = ~0u;
    u32 weightIndex = ~0u;
};

struct GltfSkinNode
{
    StringView name;
    PodVector<u32> joints;
    u32 inverseMatricesIndex = ~0u;
};

struct GltfAnimationNode
{
    struct Channel
    {
        enum class ChannelPath : u8
        {
            NONE,
            TRANSLATION,
            ROTAION,
            SCALE,
        };
        u32 samplerIndex = ~0u;
        u32 nodeIndex = ~0u;
        ChannelPath channelType = ChannelPath::NONE;
    };
    struct Sampler
    {
        enum class Interpolation : u8
        {
            NONE,
            LINEAR,
        };
        u32 inputIndex = ~0u;
        u32 outputIndex = ~0u;
        Interpolation interpolationType = Interpolation::NONE;

    };
    PodVector<Channel> channels;
    PodVector<Sampler> samplers;
    StringView name;
};

struct GltfBufferView
{
    u32 bufferIndex = ~0u;
    u32 bufferStartOffset = 0u;
    u32 bufferLength = 0u;
};

struct GltfBufferAccessor
{
    u32 bufferViewIndex = ~0u;
    u32 count = 0u;
    GltfBufferComponentCountType countType;
    GltfBufferComponentType componentType;
    PodVector<double> mins;
    PodVector<double> maxs;
    bool normalized = false;
};

struct GltfData
{
    Vector<GltfSceneNode> nodes;
    PodVector<GltfMeshNode> meshes;
    Vector<GltfSkinNode> skins;
    Vector<GltfAnimationNode> animationNodes;

    Vector<GltfBufferAccessor> accessors;
    PodVector<GltfBufferView> bufferViews;

    Vector<PodVector<u8>> buffers;
};




static u32 getSamplerIndex(const GltfData &data, u32 animationIndex, u32 nodeIndex,
    GltfAnimationNode::Channel::ChannelPath channelType)
{
    ASSERT(animationIndex < data.animationNodes.size());
    if (animationIndex >= data.animationNodes.size())
        return ~0u;
    const auto& node = data.animationNodes[animationIndex];
    for(const auto &channel : node.channels)
    {
        if(channel.nodeIndex == nodeIndex && channel.channelType == channelType)
            return channel.samplerIndex;
    }
    return ~0u;
}

template <typename T>
static bool parseAnimationChannel(const GltfData &data, u32 animationIndex,
    u32 samplerIndex, GltfBufferComponentCountType assumedCountType,
    PodVector<T> &outVector)
{
    if(samplerIndex == ~0u || animationIndex >= data.animationNodes.size())
        return false;

    u32 animationValueOffset = offsetof(T, value);
    u32 timeStampOffset = offsetof(T, timeStamp);

    const GltfAnimationNode& node = data.animationNodes[animationIndex];

    if(samplerIndex >= node.samplers.size())
        return false;
    const auto &sampler = node.samplers[samplerIndex];

    ASSERT(sampler.inputIndex < data.accessors.size());
    ASSERT(sampler.outputIndex < data.accessors.size());

    if(sampler.inputIndex >= data.accessors.size() ||
        sampler.outputIndex >= data.accessors.size())
        return false;

    const auto &timeStampAccessor = data.accessors[sampler.inputIndex];
    const auto &animationValueAccessor = data.accessors[sampler.outputIndex];

    ASSERT(timeStampAccessor.bufferViewIndex < data.bufferViews.size());
    ASSERT(animationValueAccessor.bufferViewIndex < data.bufferViews.size());
    ASSERT(timeStampAccessor.countType == GltfBufferComponentCountType::SCALAR);
    ASSERT(animationValueAccessor.countType == assumedCountType);
    ASSERT(timeStampAccessor.count == animationValueAccessor.count);

    if(timeStampAccessor.bufferViewIndex >= data.bufferViews.size() ||
        animationValueAccessor.bufferViewIndex >= data.bufferViews.size())
        return false;

    if(timeStampAccessor.countType != GltfBufferComponentCountType::SCALAR)
        return false;
    if(animationValueAccessor.countType != assumedCountType)
        return false;
    if(timeStampAccessor.count != animationValueAccessor.count)
        return false;

    outVector.resize(timeStampAccessor.count);
    ArraySliceViewBytesMutable memoryRange = sliceFromPodVectorBytesMutable(outVector);
    if(!gltfReadIntoBuffer(data, sampler.outputIndex,
        animationValueOffset, memoryRange))
        return false;
    if(!gltfReadIntoBuffer(data, sampler.inputIndex,
        timeStampOffset, memoryRange))
        return false;

    return true;
}


static u32 findAccessorIndexAndParseTimeStamps(
    const GltfData &data, u32 samplerIndex,
    PodVector<float> &inOutTimestamps)
{
    if(samplerIndex == ~0u)
        return ~0u;
    for(const GltfAnimationNode &node : data.animationNodes)
    {
        if(samplerIndex >= node.samplers.size())
            return ~0u;

        const auto &sampler = node.samplers[samplerIndex];
        if(sampler.inputIndex >= data.accessors.size() ||
            sampler.outputIndex >= data.accessors.size())
            return ~0u;

        const auto &accessor = data.accessors[sampler.inputIndex];
        if(accessor.bufferViewIndex >= data.bufferViews.size())
            return ~0u;

        if(accessor.countType != GltfBufferComponentCountType::SCALAR)
            return ~0u;

        PodVector<float> timeStamps;
        timeStamps.resize(accessor.count);

        if(!gltfReadIntoBuffer(data, sampler.inputIndex,
            0, sliceFromPodVectorBytesMutable(timeStamps)))
            return ~0u;

        // merge the timestamps
        if(inOutTimestamps.size() == 0)
            inOutTimestamps = timeStamps;
        else if(timeStamps.size() >= 0u)
        {
            u32 newTimeStampIndex = 0u;
            u32 inOutTimeStampIndex = 0u;

            if(inOutTimestamps.size() == 0 ||
                timeStamps.back() > inOutTimestamps.back())
                inOutTimestamps.pushBack(timeStamps.back());

            while(inOutTimeStampIndex <= inOutTimestamps.size()
                && newTimeStampIndex < timeStamps.size())
            {
                float t = newTimeStampIndex < timeStamps.size() ?
                    timeStamps[newTimeStampIndex] : timeStamps.back();

                inOutTimeStampIndex = inOutTimeStampIndex < inOutTimestamps.size() ?
                    inOutTimeStampIndex : inOutTimestamps.size() - 1;

                float t2 = inOutTimestamps[inOutTimeStampIndex];
                if(t <= t2)
                {
                    if(t < t2)
                        inOutTimestamps.insertIndex(inOutTimeStampIndex, t);

                    // either we added one, or they were same, so grow in those cases
                    ++newTimeStampIndex;
                }
                // either we add one, or they are same, or newtimestamp is bigger
                // so inOutTimeStampIndex grows always.
                ++inOutTimeStampIndex;
            }
        }

        return sampler.outputIndex;
    }
    return ~0u;
}

// if buffer + offsetInStruct + stride * indexcount can write past buffer......
// Reads f32, float normalized u16 and normalized u8 into float,
// Reads u32, from u8, u16, u32
// doesnt handle i8, i16 nor i32 reading, should probably just read them into i32.
static bool gltfReadIntoBuffer(const GltfData &data, u32 index,
        u32 writeStartOffsetInBytes, ArraySliceViewBytesMutable memoryRange)
{
    if(index >= data.accessors.size())
        return false;

    const GltfBufferAccessor &accessor = data.accessors [index];
    if(accessor.bufferViewIndex >= data.bufferViews.size())
        return false;

    const GltfBufferView &bufferView = data.bufferViews [accessor.bufferViewIndex];
    if(bufferView.bufferIndex >= data.buffers.size())
        return false;

    u8 *ptr = &data.buffers [bufferView.bufferIndex] [0] + bufferView.bufferStartOffset;
    const u8 *endPtr = ptr + bufferView.bufferLength;

    u32 componentCount = ~0u;

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
    u32 componentTypeBitCount = ~0u;
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

    u8 *bufferOut = memoryRange.begin() + writeStartOffsetInBytes;
    if(accessor.count == 0u)
        return false;

    // Since we write either float or u32 size of them is 4, check if we would write out of
    // buffer.
    u8 *countedLast = bufferOut + ( accessor.count - 1 ) * memoryRange.dataTypeSize +
        sizeof(u32) * componentCount;
    if(countedLast > memoryRange.end())
        return false;

    // Reading f32, normalized u16 and normalized u8 as float.
    if(accessor.componentType == GltfBufferComponentType::FLOAT_32 ||
        ( accessor.componentType == GltfBufferComponentType::UINT_16 && accessor.normalized ) ||
        ( accessor.componentType == GltfBufferComponentType::UINT_8 && accessor.normalized ))
    {
        for(u32 i = 0; i < accessor.count; ++i)
        {
            float *writeValue = ( float * )bufferOut;
            for(u32 j = 0; j < componentCount; ++j)
            {
                if(ptr + componentTypeBitCount > endPtr)
                {
                    ASSERT(false && "Trying to read past buffer end");
                    return false;
                }
                // float* ptr + 1 adds 4
                if(writeValue + 1 > ( void * )memoryRange.end())
                {
                    ASSERT(false && "Trying to write past writeBufferEnd");
                    return false;
                }

                if(accessor.componentType == GltfBufferComponentType::FLOAT_32)
                {
                    Supa::memcpy(writeValue, ptr, componentTypeBitCount);
                }
                else if(accessor.componentType == GltfBufferComponentType::UINT_16 && accessor.normalized)
                {
                    u16 tmp = 0;
                    Supa::memcpy(&tmp, ptr, componentTypeBitCount);
                    *writeValue = ( float )tmp / 65535.0f;
                }
                else if(accessor.componentType == GltfBufferComponentType::UINT_8 && accessor.normalized)
                {
                    u8 tmp = 0;
                    Supa::memcpy(&tmp, ptr, componentTypeBitCount);
                    *writeValue = ( float )tmp / 255.0f;
                }

                ++writeValue;
                ptr += componentTypeBitCount;
            }
            bufferOut += memoryRange.dataTypeSize;
        }
    }
    // i8, u8, i16, u16, i32, u32. as u32
    else if(!accessor.normalized && (
        accessor.componentType == GltfBufferComponentType::UINT_8 ||
        accessor.componentType == GltfBufferComponentType::UINT_16 ||
        accessor.componentType == GltfBufferComponentType::UINT_32 ))
        //accessor.componentType == GltfBufferComponentType::INT_8 ||
        //accessor.componentType == GltfBufferComponentType::INT_16 ||
        //accessor.componentType == GltfBufferComponentType::INT_32 ))
    {
        for(u32 i = 0; i < accessor.count; ++i)
        {
            u32 *writeValue = ( u32 * )bufferOut;
            for(u32 j = 0; j < componentCount; ++j)
            {
                if(ptr + componentTypeBitCount > endPtr)
                    return false;
                // u32* ptr + 1 adds 4
                if(writeValue + 1 > (void *)memoryRange.end())
                    return false;

                if(componentTypeBitCount == 4)
                {
                    Supa::memcpy(writeValue, ptr, componentTypeBitCount);
                }
                else if(componentTypeBitCount == 2)
                {
                    u16 tmp = 0;
                    Supa::memcpy(&tmp, ptr, componentTypeBitCount);
                    *writeValue = tmp;
                }
                else if(componentTypeBitCount == 1)
                {
                    u8 tmp = *ptr;
                    *writeValue = tmp;
                }

                ptr += componentTypeBitCount;
                ++writeValue;
            }
            bufferOut += memoryRange.dataTypeSize;
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



static bool parseMeshes(const JsonBlock &mainBlock, GltfData &data, GltfModel &outModel)
{
    const JsonBlock &meshBlock = mainBlock.getChild("meshes");
    ASSERT(meshBlock.getChildCount() >= 1);
    if(!meshBlock.isValid() || meshBlock.getChildCount() < 1)
        return false;

    u32 meshCount = meshBlock.getChildCount();
    data.meshes.resize(meshCount, GltfMeshNode());

    outModel.modelMeshes.resize(meshCount);

    for(i32 i = 0; i < meshCount; ++i)
    {
        const JsonBlock &child = meshBlock.children[i];
        GltfMeshNode &node = data.meshes[i];
        if(!child.getChild("name").parseString(node.name))
            return false;
        outModel.modelMeshes[i].meshName = SmallStackString(node.name.data(), node.name.size());
        ASSERT(child.getChild("primitives").getChildCount() == 1);
        const JsonBlock &prims = child.getChild("primitives").getChild(0);
        if(!prims.isValid())
            return false;

        if(!prims.getChild("indices").parseUInt(node.indicesIndex) ||
            !prims.getChild("material").parseUInt(node.materialIndex))
            return false;

        const JsonBlock &attribs = prims.getChild("attributes");
        if(!attribs.isValid() || attribs.getChildCount() < 1)
            return false;

        if(!attribs.getChild("POSITION").parseUInt(node.positionIndex) ||
            !attribs.getChild("NORMAL").parseUInt(node.normalIndex))
            return false;

        attribs.getChild("COLOR_0").parseUInt(node.colorIndex);

        attribs.getChild("TEXCOORD_0").parseUInt(node.uvIndex);

        attribs.getChild("JOINTS_0").parseUInt(node.jointsIndex);
        attribs.getChild("WEIGHTS_0").parseUInt(node.weightIndex);
    }
    return true;
}

static bool parseNodes(const JsonBlock &mainBlock, GltfData &data, GltfModel &outModel)
{
    const JsonBlock &nodeBlock = mainBlock.getChild("nodes");
    if(!nodeBlock.isValid() || nodeBlock.getChildCount() < 1)
        return false;

    data.nodes.resize(nodeBlock.getChildCount());
    for(i32 i = 0; i < nodeBlock.getChildCount(); ++i)
    {
        const JsonBlock &child = nodeBlock.children[i];
        GltfSceneNode &node = data.nodes[i];
        if(!child.getChild("name").parseString(node.name))
            return false;

        child.getChild("mesh").parseUInt(node.meshIndex);
        child.getChild("skin").parseUInt(node.skinIndex);


        child.getChild("rotation").parseQuat(node.rot);
        node.rot = normalize(node.rot);

        child.getChild("translation").parseVec3(node.trans);

        child.getChild("scale").parseVec3(node.scale);

        const JsonBlock &childrenBlock = child.getChild("children");
        node.childNodeIndices.reserve(childrenBlock.children.size());
        for(const auto &childBlock : childrenBlock.children)
        {
            u32 tmpIndex = ~0u;
            if(!childBlock.parseUInt(tmpIndex))
                return false;
            node.childNodeIndices.push_back(tmpIndex);
        }
    }

    return true;

}

static bool parseSkins(const JsonBlock &mainBlock, GltfData &data, GltfModel &outModel)
{
    const JsonBlock &skinBlock = mainBlock.getChild("skins");
    if(skinBlock.isValid() && skinBlock.getChildCount() > 0)
    {
        ASSERT(skinBlock.getChildCount() == 1);
        data.skins.resize(skinBlock.getChildCount());

        for(i32 i = 0; i < skinBlock.getChildCount(); ++i)
        {
            GltfSkinNode &node = data.skins[i];
            const JsonBlock &child = skinBlock.children[i];

            if(!child.getChild("name").parseString(node.name))
                return false;

            if(!child.getChild("inverseBindMatrices").parseUInt(node.inverseMatricesIndex))
                return false;

            const JsonBlock &jointsBlock = child.getChild("joints");
            if(!jointsBlock.isValid() || jointsBlock.getChildCount() < 1)
                return false;
            node.joints.reserve(jointsBlock.getChildCount());
            for(i32 j = 0; j < jointsBlock.getChildCount(); ++j)
            {
                u32 tmpInt = ~0u;
                if(!jointsBlock.getChild(j).parseUInt(tmpInt))
                    return false;

                node.joints.push_back(tmpInt);
            }

        }
    }
    return true;
}


static bool parseBuffers(const JsonBlock &mainBlock, GltfData &data, GltfModel &outModel)
{
    const JsonBlock &bufferBlock = mainBlock.getChild("buffers");
    if(!bufferBlock.isValid() || bufferBlock.getChildCount() < 1)
        return false;
    ASSERT(bufferBlock.getChildCount() == 1);

    data.buffers.resize(bufferBlock.getChildCount());

    for(i32 i = 0; i < bufferBlock.getChildCount(); ++i)
    {
        const JsonBlock &child = bufferBlock.children[i];
        PodVector<u8> &buffer = data.buffers[i];

        u32 bufLen = 0u;
        if(!child.getChild("byteLength").parseUInt(bufLen))
            return false;

        if(!child.getChild("uri").parseBuffer(buffer))
            return false;

        if(bufLen != buffer.size())
            return false;
    }


    return true;
}


static bool parseAnimations(const JsonBlock &mainBlock, GltfData &data, GltfModel &outModel)
{
    const JsonBlock &animationBlocks = mainBlock.getChild("animations");
    if(animationBlocks.isValid() && animationBlocks.getChildCount() > 0)
    {
        u32 animationCount = animationBlocks.getChildCount();
        outModel.animNames.resize(animationCount);

        data.animationNodes.resize(animationCount);
        for(u32 animIndex = 0u; animIndex < animationCount; ++animIndex)
        {
            const JsonBlock &animationBlock = animationBlocks.getChild(animIndex);
            if(!animationBlock.isValid())
                return false;
            GltfAnimationNode &animationNode = data.animationNodes[animIndex];
            if(!animationBlock.getChild("name").parseString(animationNode.name))
                return false;
            outModel.animNames[animIndex] = SmallStackString(animationNode.name.data(), animationNode.name.size());

            const JsonBlock &channelsBlock = animationBlock.getChild("channels");
            const JsonBlock &samplersBlock = animationBlock.getChild("samplers");
            if(!channelsBlock.isValid() || !samplersBlock.isValid() ||
                channelsBlock.getChildCount() == 0 || samplersBlock.getChildCount() == 0)
                return false;

            animationNode.channels.resize(channelsBlock.getChildCount());
            animationNode.samplers.resize(samplersBlock.getChildCount());

            for(u32 i = 0; i < channelsBlock.getChildCount(); ++i)
            {
                const JsonBlock &channelBlock = channelsBlock.getChild(i);
                GltfAnimationNode::Channel &channel = animationNode.channels[i];
                if(!channelBlock.getChild("sampler").parseUInt(channel.samplerIndex))
                    return false;
                if(channel.samplerIndex >= animationNode.samplers.size())
                    return false;
                if(!channelBlock.getChild("target").getChild("node").parseUInt(channel.nodeIndex))
                    return false;
                StringView pathStr;
                if(!channelBlock.getChild("target").getChild("path").parseString(pathStr))
                    return false;
                if(pathStr == "translation")
                    channel.channelType = GltfAnimationNode::Channel::ChannelPath::TRANSLATION;
                else if(pathStr == "rotation")
                    channel.channelType = GltfAnimationNode::Channel::ChannelPath::ROTAION;
                else if(pathStr == "scale")
                    channel.channelType = GltfAnimationNode::Channel::ChannelPath::SCALE;
                // unsupported?
                else
                {
                    ASSERT(false);
                    return false;
                }
            }

            for(u32 i = 0; i < samplersBlock.getChildCount(); ++i)
            {
                const JsonBlock &samplerBlock = samplersBlock.getChild(i);
                GltfAnimationNode::Sampler &sampler = animationNode.samplers[i];
                if(!samplerBlock.getChild("input").parseUInt(sampler.inputIndex))
                    return false;
                if(!samplerBlock.getChild("output").parseUInt(sampler.outputIndex))
                    return false;

                StringView interpolationStr;
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
    return true;
}


static bool parseAccessorsAndBufferViews(const JsonBlock &mainBlock, GltfData &data, GltfModel &outModel)
{
    const JsonBlock &accessorBlocks = mainBlock.getChild("accessors");
    if(!accessorBlocks.isValid() || accessorBlocks.getChildCount() < 1)
        return false;

    const JsonBlock &bufferViewBlocks = mainBlock.getChild("bufferViews");
    if(!bufferViewBlocks.isValid() || bufferViewBlocks.getChildCount() < 1)
        return false;

    data.accessors.resize(accessorBlocks.getChildCount());
    data.bufferViews.resize(bufferViewBlocks.getChildCount());

    for(u32 index = 0u; index < bufferViewBlocks.getChildCount(); ++index)
    {
        GltfBufferView &bufferView = data.bufferViews[index];
        const JsonBlock &bufferViewBlock = bufferViewBlocks.getChild(index);
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


    for(u32 index = 0u; index < accessorBlocks.getChildCount(); ++index)
    {
        GltfBufferAccessor &accessor = data.accessors[index];
        const JsonBlock &accessorBlock = accessorBlocks.getChild(index);
        if(!accessorBlock.isValid())
            return false;

        u32 componentType = ~0u;
        StringView s;
        if(!accessorBlock.getChild("bufferView").parseUInt(accessor.bufferViewIndex)
            || !accessorBlock.getChild("componentType").parseUInt(componentType)
            || !accessorBlock.getChild("count").parseUInt(accessor.count)
            || !accessorBlock.getChild("type").parseString(s)
            )
            return false;

        if(accessorBlock.getChild("sparse").isValid())
        {
            LOG("No sparse view are handled!\n");
            ASSERT(false && "No sparse view are handled");
            return false;
        }
        if(accessorBlock.getChild("min").isValid())
        {
            accessor.mins.resize(accessorBlock.getChild("min").getChildCount());
            for(u32 minIndex = 0u; minIndex < accessor.mins.size(); ++minIndex)
            {
                double value = 0.0;
                if(!accessorBlock.getChild("min").getChild(minIndex).parseNumber(value))
                    return false;
                accessor.mins[minIndex] = value;
            }
        }
        if(accessorBlock.getChild("max").isValid())
        {
            accessor.maxs.resize(accessorBlock.getChild("max").getChildCount());
            for(u32 maxIndex = 0u; maxIndex < accessor.maxs.size(); ++maxIndex)
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

        if(s == "SCALAR") accessor.countType = GltfBufferComponentCountType::SCALAR;
        else if(s == "VEC2") accessor.countType = GltfBufferComponentCountType::VEC2;
        else if(s == "VEC3") accessor.countType = GltfBufferComponentCountType::VEC3;
        else if(s == "VEC4") accessor.countType = GltfBufferComponentCountType::VEC4;
        else if(s == "MAT2") accessor.countType = GltfBufferComponentCountType::MAT2;
        else if(s == "MAT3") accessor.countType = GltfBufferComponentCountType::MAT3;
        else if(s == "MAT4") accessor.countType = GltfBufferComponentCountType::MAT4;
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
    return true;
}

static bool parseMeshData(GltfData &data, GltfModel &outModel)
{
    for(u32 i = 0; i < data.meshes.size(); ++i)
    {
        GltfModel::ModelMesh &modelMesh = outModel.modelMeshes[i];

        GltfMeshNode &node = data.meshes[i];
        if(node.positionIndex >= data.accessors.size() ||
            node.normalIndex >= data.accessors.size() ||
            node.indicesIndex >= data.accessors.size())
        {
            return false;
        }
        u32 vertexCount = data.accessors[node.positionIndex].count;
        if(vertexCount == ~0u || vertexCount == 0u ||
            vertexCount != data.accessors[node.normalIndex].count)
        {
            return false;
        }
        u32 indicesCount = data.accessors[node.indicesIndex].count;
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

        if(node.colorIndex < data.accessors.size() && vertexCount == data.accessors[node.colorIndex].count)
        {
            if(data.accessors[node.colorIndex].countType != GltfBufferComponentCountType::VEC4)
                return false;
            modelMesh.vertexColors.resize(vertexCount);
        }
        if(node.uvIndex < data.accessors.size() && vertexCount == data.accessors[node.uvIndex].count)
        {
            if(data.accessors[node.uvIndex].countType != GltfBufferComponentCountType::VEC2)
                return false;
            modelMesh.vertexUvs.resize(vertexCount);
        }
        if(data.accessors[node.indicesIndex].countType != GltfBufferComponentCountType::SCALAR)
            return false;

        modelMesh.vertices.resize(vertexCount);
        modelMesh.indices.resize(indicesCount);

        ArraySliceViewBytesMutable verticesMemoryRange = sliceFromPodVectorBytesMutable(modelMesh.vertices);
        if(!gltfReadIntoBuffer(data, node.positionIndex,
            offsetof(GltfModel::Vertex, pos),
            verticesMemoryRange))
            return false;

        if(data.accessors[node.positionIndex].mins.size() > 2)
        {
            modelMesh.bounds.min.x = data.accessors[node.positionIndex].mins[0];
            modelMesh.bounds.min.y = data.accessors[node.positionIndex].mins[1];
            modelMesh.bounds.min.z = data.accessors[node.positionIndex].mins[2];
        }
        if(data.accessors[node.positionIndex].maxs.size() > 2)
        {
            modelMesh.bounds.max.x = data.accessors[node.positionIndex].maxs[0];
            modelMesh.bounds.max.y = data.accessors[node.positionIndex].maxs[1];
            modelMesh.bounds.max.z = data.accessors[node.positionIndex].maxs[2];
        }

        if(!gltfReadIntoBuffer(data, node.normalIndex,
            offsetof(GltfModel::Vertex, norm),
            verticesMemoryRange))
            return false;

        if(node.colorIndex < data.accessors.size())
        {
            if(!gltfReadIntoBuffer(data, node.colorIndex,
                0,
                sliceFromPodVectorBytesMutable(modelMesh.vertexColors)))
                return false;
        }
        if(node.uvIndex < data.accessors.size())
        {
            if(!gltfReadIntoBuffer(data, node.uvIndex,
                0,
                sliceFromPodVectorBytesMutable(modelMesh.vertexUvs)))
                return false;
        }
        if(!gltfReadIntoBuffer(data, node.indicesIndex, 0,
            sliceFromPodVectorBytesMutable(modelMesh.indices)))
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
            modelMesh.animationVertices.resize(vertexCount);

            if(data.accessors[node.jointsIndex].countType != GltfBufferComponentCountType::VEC4)
                return false;
            if(data.accessors[node.weightIndex].countType != GltfBufferComponentCountType::VEC4)
                return false;

            if(!gltfReadIntoBuffer(data, node.weightIndex,
                offsetof(GltfModel::AnimationVertex, weights),
                sliceFromPodVectorBytesMutable(modelMesh.animationVertices)))
                return false;

            if(!gltfReadIntoBuffer(data, node.jointsIndex,
                offsetof(GltfModel::AnimationVertex, boneIndices),
                sliceFromPodVectorBytesMutable(modelMesh.animationVertices)))
                return false;
            /*
            for(const auto &v : outModel.animationVertices)
            {
                printf("boneind: [%u, %u, %u, %u], weight: [%.3f, %.3f, %.3f, %.3f]\n",
                    v.boneIndices[0], v.boneIndices[1], v.boneIndices[2], v.boneIndices[3],
                    v.weights.x, v.weights.y, v.weights.z, v.weights.w);
            }
            */
        }
    }

    return true;
}

static bool parseSkinData(GltfData &data, GltfModel &outModel)
{
    for(u32 i = 0; i < data.skins.size(); ++i)
    {
        GltfSkinNode &skinNode = data.skins[i];

        if(skinNode.inverseMatricesIndex >= data.accessors.size())
            return false;

        if(data.accessors[skinNode.inverseMatricesIndex].countType != GltfBufferComponentCountType::MAT4)
            return false;

        PodVector<Matrix> inverseMatrices;
        u32 inverseMatricesCount = data.accessors[skinNode.inverseMatricesIndex].count;
        inverseMatrices.resize(inverseMatricesCount);
        outModel.inverseNormalMatrices.resize(inverseMatricesCount);
        outModel.inverseMatrices.resize(inverseMatricesCount);

        auto calculateInverseMatrices =
            [&](auto &f, const Mat3x4 &parent, const Mat3x4 &parentNormal, u32 nodeIndex)
        {
            if(nodeIndex >= data.nodes.size())
                return;

            const auto &childNode = data.nodes[nodeIndex];
            const auto &pos = childNode.trans;
            const auto &rot = childNode.rot;
            const auto &scale = childNode.scale;

            u32 boneIndex = 0u;
            for(; boneIndex < skinNode.joints.size(); ++boneIndex)
            {
                if(skinNode.joints[boneIndex] == nodeIndex)
                    break;
            }
            if(boneIndex >= skinNode.joints.size())
                return;

            if(!isIdentity(outModel.inverseNormalMatrices[boneIndex]))
                return;

            Transform t {.pos = pos, .rot = rot, .scale = scale};
            Mat3x4 m = getModelMatrixInverse(t);
            Mat3x4 m2 = getModelNormalMatrix(t);
            Mat3x4 m3 = getModelMatrix(t);

            Mat3x4 newParent = m * parent;
            Mat3x4 newParentNormal = parentNormal * m2;

            outModel.inverseMatrices[boneIndex] = newParent;
            outModel.inverseNormalMatrices[boneIndex] = newParentNormal;

            for(u32 childNodeIndex : childNode.childNodeIndices)
            {
                f(f, newParent, newParentNormal, childNodeIndex);
            }
        };
        for(u32 jointIndex : skinNode.joints)
            calculateInverseMatrices(calculateInverseMatrices, Mat3x4(), Mat3x4(), jointIndex);

        if(!gltfReadIntoBuffer(data, skinNode.inverseMatricesIndex,
            0, sliceFromPodVectorBytesMutable(inverseMatrices)))
            return false;
        //printf("\n\n");
        for(u32 i = 0; i < inverseMatrices.size(); ++i)
        {
            inverseMatrices[i] = transpose(inverseMatrices[i]);;
            outModel.inverseMatrices[i] = inverseMatrices[i];
            /*
            u32 nodeJointIndex = skinNode.joints[i];
            const auto &node = data.nodes[nodeJointIndex];
            printf("Index: %u, joint: %u, name: %s\n", i, nodeJointIndex, String(node.name).getStr());
            printVector3(node.trans, "Pos");
            printQuaternion(node.rot, "Rot");
            printVector3(node.scale, "Scale");
            printMatrix(inverseMatrices[i], " invmat");
            printMatrix(inverse(inverseMatrices[i]), "inverse of invmat");
            printMatrix(mat1[nodeJointIndex], "mat1");
            printMatrix(mat2[nodeJointIndex], "mat2");
            //printMatrix(mat3[nodeJointIndex], "mat3");
            printf("\n\n\n");
            */

            //ASSERT(inverse(inverseMatrices[i]) == mat1[nodeJointIndex]);
        }
    }
    return true;
}

static bool parseAnimationData(GltfData &data, GltfModel &outModel)
{
    if(data.skins.getSize() == 1 && data.skins[0].joints.size() > 0)
    {
        const auto &joints = data.skins[0].joints;
        u32 jointCount = joints.size();

        outModel.animationIndices.resize(data.animationNodes.size());
        outModel.animStartTimes.resize(data.animationNodes.size());
        outModel.animEndTimes.resize(data.animationNodes.size());

        for(u32 animationIndex = 0; animationIndex < data.animationNodes.getSize(); ++animationIndex)
        {
            outModel.animationIndices[animationIndex].resize(jointCount);
            for(u32 jointIndex = 0u; jointIndex < jointCount; ++jointIndex)
            {
                u32 nodeIndex = joints[jointIndex];
                ASSERT(nodeIndex < data.nodes.size());
                if(nodeIndex >= data.nodes.size())
                    return false;

                auto &animationBoneIndices = outModel.animationIndices[animationIndex][jointIndex];

                u32 positionSamplerIndex = getSamplerIndex(data, animationIndex, nodeIndex,
                    GltfAnimationNode::Channel::ChannelPath::TRANSLATION);
                u32 rotationSamplerIndex = getSamplerIndex(data, animationIndex, nodeIndex,
                    GltfAnimationNode::Channel::ChannelPath::ROTAION);
                u32 scaleSamplerIndex = getSamplerIndex(data, animationIndex, nodeIndex,
                    GltfAnimationNode::Channel::ChannelPath::SCALE);

                // parse positions
                {
                    PodVector<GltfModel::AnimPos> bonePosVector;

                    if(!parseAnimationChannel(data, animationIndex, positionSamplerIndex,
                        GltfBufferComponentCountType::VEC3, bonePosVector))
                    {
                        ASSERT(false && "failed to parse animation channel for position");
                        return false;
                    }
                    /*
                    for(const auto &v : bonePosVector)
                    {
                        printf("time: %f, pos: [%f, %f, %f]\n",
                            v.timeStamp, v.value.x, v.value.y, v.value.z);
                    }
                    */

                    animationBoneIndices.posStartIndex = outModel.animationPosData.size();
                    animationBoneIndices.posIndexCount = bonePosVector.size();

                    outModel.animationPosData.pushBack(bonePosVector);
                }
                // parse rot
                {
                    PodVector<GltfModel::AnimRot> boneRotVector;

                    if(!parseAnimationChannel(data, animationIndex, rotationSamplerIndex,
                        GltfBufferComponentCountType::VEC4, boneRotVector))
                    {
                        ASSERT(false && "failed to parse animation channel for rotation");
                        return false;
                    }
                    /*
                    for(const auto &v : boneRotVector)
                    {
                        printf("time: %f, rot: [%f, %f, %f], [%f]\n",
                            v.timeStamp, v.value.v.x, v.value.v.y, v.value.v.z, v.value.w);
                    }
                    */

                    animationBoneIndices.rotStartIndex = outModel.animationRotData.size();
                    animationBoneIndices.rotIndexCount = boneRotVector.size();

                    outModel.animationRotData.pushBack(boneRotVector);
                }
                // parse scale
                {
                    PodVector<GltfModel::AnimScale> boneScaleVector;

                    if(!parseAnimationChannel(data, animationIndex, scaleSamplerIndex,
                        GltfBufferComponentCountType::VEC3, boneScaleVector))
                    {
                        ASSERT(false && "failed to parse animation channel for scale");
                        return false;
                    }
                    /*
                    for(const auto &v : boneScaleVector)
                    {
                        printf("time: %f, scale: [%f, %f, %f]\n",
                            v.timeStamp, v.value.x, v.value.y, v.value.z);
                    }
                    */

                    animationBoneIndices.scaleStartIndex = outModel.animationScaleData.size();
                    animationBoneIndices.scaleIndexCount = boneScaleVector.size();
                    outModel.animationScaleData.pushBack(boneScaleVector);
                }

                // remap children
                {
                    const auto &childNodes = data.nodes[nodeIndex].childNodeIndices;
                    animationBoneIndices.childStartIndex = outModel.childrenJointIndices.size();
                    u32 count = 0u;
                    for(u32 childNodeIndex : childNodes)
                    {
                        bool found = false;
                        for(u32 childJointIndex = 0u; childJointIndex < joints.size(); ++childJointIndex)
                        {
                            if(joints[childJointIndex] == childNodeIndex)
                            {
                                outModel.childrenJointIndices.push_back(childJointIndex);
                                ++count;
                                found = true;
                                break;
                            }
                        }
                        ASSERT(found);
                        if(!found)
                            return false;
                    }
                    animationBoneIndices.childIndexCount = count;
                }
            }
        }

        for(u32 animationIndex = 0; animationIndex < data.animationNodes.getSize(); ++animationIndex)
        {
            float animStartTime = 1e10;
            float animEndTime = -1e10;
            ASSERT(animationIndex < outModel.animationIndices.size());

            if(animationIndex >= outModel.animationIndices.size())
                return false;
            const auto &animationVector = outModel.animationIndices[animationIndex];
            for(const auto &animNode : animationVector)
            {
                for(u32 ind = animNode.posStartIndex; ind < animNode.posStartIndex + animNode.posIndexCount; ++ind)
                {
                    animStartTime = Supa::minf(animStartTime, outModel.animationPosData[ind].timeStamp);
                    animEndTime = Supa::maxf(animEndTime, outModel.animationPosData[ind].timeStamp);
                }

                for(u32 ind = animNode.rotStartIndex; ind < animNode.rotStartIndex + animNode.rotIndexCount; ++ind)
                {
                    animStartTime = Supa::minf(animStartTime, outModel.animationRotData[ind].timeStamp);
                    animEndTime = Supa::maxf(animEndTime, outModel.animationRotData[ind].timeStamp);
                }

                for(u32 ind = animNode.scaleStartIndex; ind < animNode.scaleStartIndex + animNode.scaleIndexCount; ++ind)
                {
                    animStartTime = Supa::minf(animStartTime, outModel.animationScaleData[ind].timeStamp);
                    animEndTime = Supa::maxf(animEndTime, outModel.animationScaleData[ind].timeStamp);
                }
            }
            outModel.animStartTimes[animationIndex] = animStartTime;
            outModel.animEndTimes[animationIndex] = animEndTime;
        }
    }
    return true;
}

bool readGLTF(const char *filename, GltfModel &outModel)
{
    GltfData data;

    PodVector<char> buffer;

    if(!loadBytes(filename, buffer.getBuffer()))
        return false;

    JsonBlock mainBlock;
    bool parseSuccess = mainBlock.parseJson(StringView(buffer.data(), buffer.size()));

    if (!parseSuccess)
    {
        printf("Failed to parse: %s\n", filename);
        return false;
    }
    else
    {
        //bl.print();
    }

    if(!mainBlock.isObject() || mainBlock.getChildCount() < 1)
        return false;


    // Parse meshes
    if(!parseMeshes(mainBlock, data, outModel))
        return false;

    if(!parseNodes(mainBlock, data, outModel))
        return false;

    if(!parseSkins(mainBlock, data, outModel))
        return false;

    if(!parseBuffers(mainBlock, data, outModel))
        return false;

    if(!parseAnimations(mainBlock, data, outModel))
        return false;

    if(!parseAnimations(mainBlock, data, outModel))
        return false;

    if(!parseAccessorsAndBufferViews(mainBlock, data, outModel))
        return false;

    if(!parseMeshData(data, outModel))
        return false;

    if(!parseSkinData(data, outModel))
        return false;
    if(!parseAnimationData(data, outModel))
        return false;


/*
    const ArraySliceView<GltfModel::Vertex> vertices(&outModel.vertices[0], outModel.vertices.size());
    for(u32 i = 0; i < vertices.size(); ++i)
    {
        printf("pos vert: %i:   x: %f, y: %f, z: %f\n", i, vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z);
        printf("nor vert: %i:   x: %f, y: %f, z: %f\n", i, vertices[i].norm.x, vertices[i].norm.y, vertices[i].norm.z);
        printf("col vert: %i:   x: %f, y: %f, z: %f, w: %f\n", i, vertices[i].color.x, vertices[i].color.y, vertices[i].color.z, vertices[i].color.w);
    }

    const ArraySliceView<GltfModel::AnimationVertex> animationVertices(
        &outModel.animationVertices[0], outModel.animationVertices.size());
    for(u32 i = 0; i < animationVertices.size(); ++i)
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

    const ArraySliceView<u32> indices(&outModel.indices[0], outModel.indices.size());
    for(u32 i = 0; i < indices.size(); ++i)
    {
        printf("i: %i, index: %u\n", i, indices[i]);
    }

    for(u32 i = 0; i < data.skins.size(); ++i)
    {
        const GltfSkinNode &skin = data.skins[i];
        printf("Skin node: %u\n", i);
        for(u32 j = 0; j < skin.inverseMatrices.size(); ++j)
        {
            printf("Matrix: %i\n", j);
            printMatrix(skin.inverseMatrices[j], "");
        }
    }
*/
    return true;
}

struct EvaluateBoneParams
{
    const ArraySliceView< GltfModel::AnimationIndexData > animationData;
    //const ArraySliceView < float > times;
    const ArraySliceView< u32 > childIndices;
    const ArraySliceView< GltfModel::AnimPos > posses;
    const ArraySliceView< GltfModel::AnimRot > rots;
    const ArraySliceView< GltfModel::AnimScale > scales;
    const ArraySliceView< Mat3x4> inverseMatrices;
    const ArraySliceView< Mat3x4> inverseNormalMatrices;
};

static bool interpolateBetweenPoses(const EvaluateBoneParams &params, u32 jointIndex, float weight, float time,
    Transform &inOutTransform)
{
    if(jointIndex >= params.animationData.size())
        return false;
    const auto &animData = params.animationData[jointIndex];

    Vec3 pos{ Uninit };
    Quat rot{ Uninit };
    Vec3 scale{ Uninit };

    auto getInterpolationValue = [](float prevTime, float nextTime, float currTime)
    {
        currTime = Supa::maxf(prevTime, currTime);
        currTime = Supa::minf(nextTime, currTime);
        float duration = (nextTime - prevTime);

        float frac = duration > 0.0f ? (currTime - prevTime) / duration : 1.0f;
        return frac;
    };


    if(animData.posIndexCount > 0)
    {
        const auto *curr = &params.posses[0];
        const auto *next = curr;
        float frac = 1.0f;
        const ArraySliceView< GltfModel::AnimPos > sliceView(
            params.posses, animData.posStartIndex + 1, animData.posIndexCount);

        for(const auto &nextPos : sliceView)
        {
            curr = next;
            next = &nextPos;
            if(time <= next->timeStamp)
            {
                float frac = getInterpolationValue(curr->timeStamp, next->timeStamp, time);
                break;
            }
        }
        //pos = lerp(curr->value, next->value, frac);
        pos.x = curr->value.x + (next->value.x - curr->value.x) * frac;
        pos.y = curr->value.y + (next->value.y - curr->value.y) * frac;
        pos.z = curr->value.z + (next->value.z - curr->value.z) * frac;
    }

    if(animData.rotIndexCount > 0)
    {
        const auto *curr = &params.rots[0];
        const auto *next = curr;
        float frac = 1.0f;
        const ArraySliceView< GltfModel::AnimRot > sliceView(
            params.rots, animData.rotStartIndex + 1, animData.rotIndexCount);

        for(const auto &nextPos : sliceView)
        {
            curr = next;
            next = &nextPos;
            if(time <= next->timeStamp)
            {
                float frac = getInterpolationValue(curr->timeStamp, next->timeStamp, time);
                break;
            }
        }
        rot = normalize(lerp(curr->value, next->value, frac));
    }

    if(animData.scaleIndexCount > 0)
    {
        const auto *curr = &params.scales[0];
        const auto *next = curr;
        float frac = 1.0f;
        const ArraySliceView< GltfModel::AnimScale > sliceView(
            params.scales, animData.scaleStartIndex + 1, animData.scaleIndexCount);

        for(const auto &nextPos : sliceView)
        {
            curr = next;
            next = &nextPos;
            if(time <= next->timeStamp)
            {
                float frac = getInterpolationValue(curr->timeStamp, next->timeStamp, time);
                break;
            }
        }
        scale.x = curr->value.x + (next->value.x - curr->value.x) * frac;
        scale.y = curr->value.y + (next->value.y - curr->value.y) * frac;
        scale.z = curr->value.z + (next->value.z - curr->value.z) * frac;
    }

    inOutTransform.pos = inOutTransform.pos + pos * weight;
    inOutTransform.scale = inOutTransform.scale + scale * weight;
    if(dot(inOutTransform.rot, rot) < 0)
    {
        rot.v = -rot.v;
        rot.w = -rot.w;
    }
    inOutTransform.rot.v.x = inOutTransform.rot.v.x + rot.v.x * weight;
    inOutTransform.rot.v.y = inOutTransform.rot.v.y + rot.v.y * weight;
    inOutTransform.rot.v.z = inOutTransform.rot.v.z + rot.v.z * weight;
    inOutTransform.rot.w = inOutTransform.rot.w + rot.w * weight;
    return true;
}

static bool evaluateBone(const EvaluateBoneParams &params, u32 jointIndex,
    const Mat3x4 &parentMatrix, const Mat3x4 &parentNormalMatrix,
    ArraySliceView<Transform> transforms, ArraySliceViewMutable<Mat3x4> outMatrices)
{
    if(jointIndex >= params.animationData.size())
        return false;
    const auto &animData = params.animationData[jointIndex];
    if(animData.posIndexCount == 0 || animData.rotIndexCount == 0 || animData.scaleIndexCount == 0)
    {
        outMatrices[jointIndex] = Mat3x4();
        return false;
    }

    Mat3x4 newParent = parentMatrix * getModelMatrix(transforms[jointIndex]);
    Mat3x4 newParentNormal = parentNormalMatrix * getModelNormalMatrix(transforms[jointIndex]);
    outMatrices[jointIndex * 2] =  newParent * params.inverseMatrices[jointIndex];
    outMatrices[jointIndex * 2 + 1] = newParentNormal * params.inverseMatrices[jointIndex];// params.inverseNormalMatrices[jointIndex];
    const ArraySliceView< u32 > childIndices(
        params.childIndices, animData.childStartIndex, animData.childIndexCount);
    for(u32 childIndex : childIndices)
    {
        bool success = evaluateBone(params, childIndex, newParent, newParentNormal, transforms, outMatrices);
        if(!success)
            return false;
    }
    return true;
}

bool evaluateAnimation(const GltfModel &model, u32 animationIndex, float time,
    PodVector<Mat3x4> &outMatrices)
{
    AnimationState state;
    state.activeIndices = 1;
    state.animationIndices[0] = animationIndex;
    state.blendValues[0] = 1.0f;
    state.time[0] = time;

    return evaluateAnimation(model, state, outMatrices);
}


bool evaluateAnimation(const GltfModel &model, AnimationState &animationState,
    PodVector<Mat3x4> &outMatrices)
{
    if(model.animationIndices.size() == 0)
        return false;

    if(model.inverseMatrices.size() > 255)
        return false;
    outMatrices.uninitializedResize(model.inverseMatrices.getSize() * 2);

    PodVector<Transform> transforms;
    transforms.uninitializedResize(model.inverseMatrices.getSize());
    auto mutableTransforms = sliceFromPodVectorMutable(transforms);
    for(u32 index = 0; index < mutableTransforms.size(); ++index)
    {
        auto &t = mutableTransforms[index];
        Supa::memset(&t, 0, sizeof(Transform));
    }
    float totalWeight = 0.0f;
    for(u32 i = 0; i < AnimationState::AMOUNT; ++i)
    {
        if(((animationState.activeIndices >> i) & 1) != 1)
            continue;

        u32 animationIndex = animationState.animationIndices[i];
        float time = animationState.time[i];
        float weight = animationState.blendValues[i];
        if(weight == 0.0f)
            continue;
        animationIndex = animationIndex < model.animationIndices.size() ? animationIndex : model.animationIndices.size() - 1;

        if(animationIndex >= model.animationIndices.size())
            return false;

        const EvaluateBoneParams params{
            .animationData = sliceFromPodVector(model.animationIndices[animationIndex]),
            .childIndices = sliceFromPodVector(model.childrenJointIndices),
            .posses = sliceFromPodVector(model.animationPosData),
            .rots = sliceFromPodVector(model.animationRotData),
            .scales = sliceFromPodVector(model.animationScaleData),
            .inverseMatrices = sliceFromPodVector(model.inverseMatrices),
            .inverseNormalMatrices = sliceFromPodVector(model.inverseNormalMatrices),
        };

        float animStartTime = model.animStartTimes[animationIndex];
        float animEndTime = model.animEndTimes[animationIndex];
        float duration = animEndTime - animStartTime;
        ASSERT(duration != 0.0f);
        float div = duration > 0.0 ? time / duration : 0.0;
        time -= i64(div) * duration;
        while(time < animStartTime)
            time += duration;
        while(time > animEndTime)
            time -= duration;

        totalWeight += weight;

        for(u32 index = 0; index < mutableTransforms.size(); ++index)
        {
            auto &t = mutableTransforms[index];
            if(!interpolateBetweenPoses(params, index, weight, time, t))
                return false;
        }
    }

    for(u32 index = 0; index < mutableTransforms.size(); ++index)
    {
        auto &t = mutableTransforms[index];
        if(totalWeight == 0.0f)
            t = Transform();
        else
        {
            t.pos = t.pos / totalWeight;
            t.scale = t.scale / totalWeight;
            t.rot.v.x = t.rot.v.x / totalWeight;
            t.rot.v.y = t.rot.v.y / totalWeight;
            t.rot.v.z = t.rot.v.z / totalWeight;
            t.rot.w = t.rot.w / totalWeight;
            t.rot = normalize(t.rot);
            //printVector3(t.pos, "pos");
            //printQuaternion(t.rot, "rot");
            //printVector3(t.scale, "scale");
        }
    }


    for(u32 i = 0; i < AnimationState::AMOUNT; ++i)
    {
        if(((animationState.activeIndices >> i) & 1) != 1)
            continue;

        u32 animationIndex = animationState.animationIndices[i];
        float time = animationState.time[i];
        float weight = animationState.blendValues[i];

        animationIndex = animationIndex < model.animationIndices.size() ? animationIndex : model.animationIndices.size() - 1;

        if(animationIndex >= model.animationIndices.size())
            return false;

        const EvaluateBoneParams params{
            .animationData = sliceFromPodVector(model.animationIndices[animationIndex]),
            .childIndices = sliceFromPodVector(model.childrenJointIndices),
            .posses = sliceFromPodVector(model.animationPosData),
            .rots = sliceFromPodVector(model.animationRotData),
            .scales = sliceFromPodVector(model.animationScaleData),
            .inverseMatrices = sliceFromPodVector(model.inverseMatrices),
            .inverseNormalMatrices = sliceFromPodVector(model.inverseNormalMatrices),
        };

        if(!evaluateBone(params, 0, Mat3x4(), Mat3x4(),
            sliceFromPodVector(transforms), sliceFromPodVectorMutable(outMatrices)))
            return false;
    }
    return true;
}
