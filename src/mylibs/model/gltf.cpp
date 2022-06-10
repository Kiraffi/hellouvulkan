#include "gltf.h"

#include <container/arraysliceview.h>

#include <core/general.h>
#include <core/json.h>
#include <core/transform.h>

#include <math/matrix.h>
#include <math/quaternion.h>

#include <core/timer.h>

struct GltfData;

static bool gltfReadIntoBuffer(const GltfData &data, uint32_t index,
        uint32_t writeStartOffsetInBytes, ArraySliceViewBytes memoryRange);

static uint32_t findAccessorIndexAndParseTimeStamps(
    const GltfData &data, uint32_t samplerIndex,
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
    std::string_view name;
    Quat rot;
    Vec3 trans;
    Vec3 scale{1.0f, 1.0f, 1.0f};
    uint32_t meshIndex = ~0u;
    uint32_t skinIndex = ~0u;
    PodVector<uint32_t> childNodeIndices;
};

struct GltfMeshNode
{
    std::string_view name;
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
    std::string_view name;
    PodVector<uint32_t> joints;
    uint32_t inverseMatricesIndex = ~0u;
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
        ChannelPath channelType = ChannelPath::NONE;
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
    PodVector<Channel> channels;
    PodVector<Sampler> samplers;
    std::string_view name;
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

    Vector<PodVector<uint8_t>> buffers;
};




static uint32_t getSamplerIndex(const GltfData &data, uint32_t animationIndex, uint32_t nodeIndex,
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
static bool parseAnimationChannel(const GltfData &data, uint32_t animationIndex,
    uint32_t samplerIndex, GltfBufferComponentCountType assumedCountType,
    PodVector<T> &outVector)
{
    if(samplerIndex == ~0u || animationIndex >= data.animationNodes.size())
        return false;

    uint32_t animationValueOffset = offsetof(T, value);
    uint32_t timeStampOffset = offsetof(T, timeStamp);

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
    ArraySliceViewBytes memoryRange = sliceFromPodVectorBytes(outVector);
    if(!gltfReadIntoBuffer(data, sampler.outputIndex,
        animationValueOffset, memoryRange))
        return false;
    if(!gltfReadIntoBuffer(data, sampler.inputIndex,
        timeStampOffset, memoryRange))
        return false;

    return true;
}


static uint32_t findAccessorIndexAndParseTimeStamps(
    const GltfData &data, uint32_t samplerIndex,
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
            0, sliceFromPodVectorBytes(timeStamps)))
            return ~0u;

        // merge the timestamps
        if(inOutTimestamps.size() == 0)
            inOutTimestamps = timeStamps;
        else if(timeStamps.size() >= 0u)
        {
            uint32_t newTimeStampIndex = 0u;
            uint32_t inOutTimeStampIndex = 0u;

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
static bool gltfReadIntoBuffer(const GltfData &data, uint32_t index,
        uint32_t writeStartOffsetInBytes, ArraySliceViewBytes memoryRange)
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

    uint8_t *bufferOut = memoryRange.begin() + writeStartOffsetInBytes;
    if(accessor.count == 0u)
        return false;

    // Since we write either float or uint32_t size of them is 4, check if we would write out of
    // buffer.
    uint8_t *countedLast = bufferOut + ( accessor.count - 1 ) * memoryRange.dataTypeSize +
        sizeof(uint32_t) * componentCount;
    if(countedLast > memoryRange.end())
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
                if(writeValue + 1 > ( void * )memoryRange.end())
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
            bufferOut += memoryRange.dataTypeSize;
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
                if(writeValue + 1 > (void *)memoryRange.end())
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






bool readGLTF(std::string_view filename, GltfModel &outModel)
{
    GltfData data;

    PodVector<char> buffer;

    if(!loadBytes(filename, buffer))
        return false;

    JSONBlock bl;
    bool parseSuccess = bl.parseJSON(ArraySliceView(buffer.data(), buffer.size()));

    if (!parseSuccess)
    {
        printf("Failed to parse: %s\n", filename.data());
        return false;
    }
    else
    {
        //bl.print();
    }

    if(!bl.isObject() || bl.getChildCount() < 1)
        return false;


    // Parse meshes
    {
        const JSONBlock &meshBlock = bl.getChild("meshes");
        ASSERT(meshBlock.getChildCount() == 1);
        if(!meshBlock.isValid() || meshBlock.getChildCount() != 1)
            return false;

        data.meshes.resize(meshBlock.getChildCount(), GltfMeshNode());

        for(int i = 0; i < meshBlock.getChildCount(); ++i)
        {
            const JSONBlock &child = meshBlock.children [i];
            GltfMeshNode &node = data.meshes [i];
            if(!child.getChild("name").parseString(node.name))
                return false;
            ASSERT(child.getChild("primitives").getChildCount() == 1);
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
                !attribs.getChild("NORMAL").parseUInt(node.normalIndex))
                return false;

            attribs.getChild("COLOR_0").parseUInt(node.colorIndex);

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
            node.rot = normalize(node.rot);

            const JSONBlock &transBlock = child.getChild("translation");
            transBlock.getChild(0).parseNumber(node.trans.x);
            transBlock.getChild(1).parseNumber(node.trans.y);
            transBlock.getChild(2).parseNumber(node.trans.z);

            const JSONBlock &scaleBlock = child.getChild("scale");
            scaleBlock.getChild(0).parseNumber(node.scale.x);
            scaleBlock.getChild(1).parseNumber(node.scale.y);
            scaleBlock.getChild(2).parseNumber(node.scale.z);

            const JSONBlock& childrenBlock = child.getChild("children");
            node.childNodeIndices.reserve(childrenBlock.children.size());
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
            ASSERT(skinBlock.getChildCount() == 1);
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
                node.joints.reserve(jointsBlock.getChildCount());
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
        ASSERT(bufferBlock.getChildCount() == 1);

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
                    std::string_view pathStr;
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

                for(uint32_t i = 0; i < samplersBlock.getChildCount(); ++i)
                {
                    const JSONBlock &samplerBlock = samplersBlock.getChild(i);
                    GltfAnimationNode::Sampler &sampler = animationNode.samplers[i];
                    if(!samplerBlock.getChild("input").parseUInt(sampler.inputIndex))
                        return false;
                    if(!samplerBlock.getChild("output").parseUInt(sampler.outputIndex))
                        return false;

                    std::string_view interpolationStr;
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
            std::string_view s;
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
            node.indicesIndex >= data.accessors.size())
        {
            return false;
        }
        uint32_t vertexCount = data.accessors[node.positionIndex].count;
        if(vertexCount == ~0u || vertexCount == 0u ||
            vertexCount != data.accessors[node.normalIndex].count )
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

        if (node.colorIndex < data.accessors.size() && vertexCount == data.accessors[node.colorIndex].count)
        {
            if (data.accessors[node.colorIndex].countType != GltfBufferComponentCountType::VEC4)
                return false;
            outModel.vertexColors.resize(vertexCount);
        }
        if (node.uvIndex < data.accessors.size() && vertexCount == data.accessors[node.uvIndex].count)
        {
            if (data.accessors[node.uvIndex].countType != GltfBufferComponentCountType::VEC2)
                return false;
            outModel.vertexUvs.resize(vertexCount);
        }
        if(data.accessors[node.indicesIndex].countType != GltfBufferComponentCountType::SCALAR)
            return false;

        outModel.vertices.resize(vertexCount);
        outModel.indices.resize(indicesCount);

        ArraySliceViewBytes verticesMemoryRange = sliceFromPodVectorBytes(outModel.vertices);
        if(!gltfReadIntoBuffer(data, node.positionIndex,
            offsetof(GltfModel::Vertex, pos),
            verticesMemoryRange))
            return false;

        if(data.accessors[node.positionIndex].mins.size() > 2)
        {
            outModel.bounds.min.x = data.accessors[node.positionIndex].mins[0];
            outModel.bounds.min.y = data.accessors[node.positionIndex].mins[1];
            outModel.bounds.min.z = data.accessors[node.positionIndex].mins[2];
        }
        if(data.accessors[node.positionIndex].maxs.size() > 2)
        {
            outModel.bounds.max.x = data.accessors[node.positionIndex].maxs[0];
            outModel.bounds.max.y = data.accessors[node.positionIndex].maxs[1];
            outModel.bounds.max.z = data.accessors[node.positionIndex].maxs[2];
        }

        if(!gltfReadIntoBuffer(data, node.normalIndex,
            offsetof(GltfModel::Vertex, norm),
            verticesMemoryRange))
            return false;

        if (node.colorIndex < data.accessors.size())
        {
            if (!gltfReadIntoBuffer(data, node.colorIndex,
                0,
                sliceFromPodVectorBytes(outModel.vertexColors)))
                return false;
        }
        if (node.uvIndex < data.accessors.size())
        {
            if (!gltfReadIntoBuffer(data, node.uvIndex,
                0,
                sliceFromPodVectorBytes(outModel.vertexUvs)))
                return false;
        }
        if(!gltfReadIntoBuffer(data, node.indicesIndex, 0,
            sliceFromPodVectorBytes(outModel.indices)))
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
                offsetof(GltfModel::AnimationVertex, weights),
                sliceFromPodVectorBytes(outModel.animationVertices)))
                return false;

            if(!gltfReadIntoBuffer(data, node.jointsIndex,
                offsetof(GltfModel::AnimationVertex, boneIndices),
                sliceFromPodVectorBytes(outModel.animationVertices)))
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



        for(uint32_t i = 0; i < data.skins.size(); ++i)
        {
            GltfSkinNode &skinNode = data.skins[i];

            if(skinNode.inverseMatricesIndex >= data.accessors.size())
                return false;

            if(data.accessors[skinNode.inverseMatricesIndex].countType != GltfBufferComponentCountType::MAT4)
                return false;

            PodVector<Matrix> mat1;
            PodVector<Matrix> mat2;
            PodVector<Matrix> mat3;
            mat1.resize(data.nodes.size());
            mat2.resize(data.nodes.size());
            mat3.resize(data.nodes.size());

            auto fu = [&](auto &f, const Matrix &parent, const Matrix &invParent, uint32_t boneIndex)
            {
                if(boneIndex >= data.nodes.size())
                    return;

                const auto &childNode = data.nodes[boneIndex];
                const auto &pos = childNode.trans;
                const auto &rot = childNode.rot;
                const auto &scale = childNode.scale;
                //printf("boneindex: %u - before\n", boneIndex);
                if(!isIdentity(mat1[boneIndex]))
                    return;
                //printf("boneindex: %u - after\n", boneIndex);
                Transform t{ .pos = pos, .rot = rot, .scale = scale };
                Matrix m = getModelMatrix(t);
                Matrix m2 = getModelMatrixInverse(t);
                //Matrix newParent = m * parent;
                Matrix newParent = parent * m;
                Matrix newInverseParent = m2 * invParent;
                mat1[boneIndex] = newParent;
                mat2[boneIndex] = inverse(newParent);
                mat3[boneIndex] = newInverseParent;
                ASSERT(isIdentity(newParent * inverse(newParent)));
                ASSERT(isIdentity(inverse(newParent) * newParent));

                for(uint32_t childNodeIndex : childNode.childNodeIndices)
                {
                    f(f, newParent, newInverseParent, childNodeIndex);
                }
            };
            for(uint32_t jointIndex : skinNode.joints)
                fu(fu, Matrix(), Matrix(), jointIndex);

            PodVector<Matrix> &inverseMatrices = outModel.inverseMatrices;
            inverseMatrices.resize(data.accessors[skinNode.inverseMatricesIndex].count);

            if(!gltfReadIntoBuffer(data, skinNode.inverseMatricesIndex,
                0, sliceFromPodVectorBytes(inverseMatrices)))
                return false;
            //printf("\n\n");
            for(uint32_t i = 0; i < inverseMatrices.size(); ++i)
            {
                uint32_t nodeJointIndex = skinNode.joints[i];

                const auto &node = data.nodes[nodeJointIndex];
                inverseMatrices[i] = transpose(inverseMatrices[i]);
                /*
                printf("Index: %u, joint: %u, name: %s\n", i, nodeJointIndex, std::string(node.name).c_str());
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

        // Parse animationdata
        if (data.skins.getSize() == 1 && data.skins[0].joints.size() > 0)
        {
            uint32_t jointCount = data.skins[0].joints.size();
            outModel.animationPosData.resize(data.animationNodes.getSize());
            outModel.animationRotData.resize(data.animationNodes.getSize());
            outModel.animationScaleData.resize(data.animationNodes.getSize());
            outModel.animStartTimes.resize(data.animationNodes.size());
            outModel.animEndTimes.resize(data.animationNodes.size());

            outModel.bones.resize(jointCount);
            for (uint32_t boneIndex = 0u; boneIndex < jointCount; ++boneIndex)
            {
                uint32_t jointIndex = data.skins[0].joints[boneIndex];
                ASSERT(jointIndex < data.nodes.size());
                if (jointIndex >= data.nodes.size())
                    return false;
                for (uint32_t animationIndex = 0; animationIndex < data.animationNodes.getSize(); ++animationIndex)
                {
                    outModel.animationPosData[animationIndex].resize(jointCount);
                    outModel.animationRotData[animationIndex].resize(jointCount);
                    outModel.animationScaleData[animationIndex].resize(jointCount);


                    PodVector<GltfModel::BoneAnimationPosOrScale>& bonePosVector
                        = outModel.animationPosData[animationIndex][boneIndex];
                    PodVector<GltfModel::BoneAnimationRot>& boneRotVector
                        = outModel.animationRotData[animationIndex][boneIndex];
                    PodVector<GltfModel::BoneAnimationPosOrScale>& boneScaleVector
                        = outModel.animationScaleData[animationIndex][boneIndex];

                    uint32_t positionSamplerIndex = getSamplerIndex(data, animationIndex, jointIndex,
                        GltfAnimationNode::Channel::ChannelPath::TRANSLATION);
                    uint32_t rotationSamplerIndex = getSamplerIndex(data, animationIndex, jointIndex,
                        GltfAnimationNode::Channel::ChannelPath::ROTAION);
                    uint32_t scaleSamplerIndex = getSamplerIndex(data, animationIndex, jointIndex,
                        GltfAnimationNode::Channel::ChannelPath::SCALE);


                    if (!parseAnimationChannel(data, animationIndex, positionSamplerIndex,
                        GltfBufferComponentCountType::VEC3, bonePosVector))
                    {
                        ASSERT(false && "failed to parse animation channel for position");
                    }
                    if (!parseAnimationChannel(data, animationIndex, rotationSamplerIndex,
                        GltfBufferComponentCountType::VEC4, boneRotVector))
                    {
                        ASSERT(false && "failed to parse animation channel for position");
                    }
                    if (!parseAnimationChannel(data, animationIndex, scaleSamplerIndex,
                        GltfBufferComponentCountType::VEC3, boneScaleVector))
                    {
                        ASSERT(false && "failed to parse animation channel for position");
                    }

                    /*
                    for(const auto &v : bonePosVector)
                    {
                        printf("time: %f, pos: [%f, %f, %f]\n",
                            v.timeStamp, v.value.x, v.value.y, v.value.z);
                    }
                    */
                    /*
                    for(const auto &v : boneRotVector)
                    {
                        printf("time: %f, rot: [%f, %f, %f], [%f]\n",
                            v.timeStamp, v.value.v.x, v.value.v.y, v.value.v.z, v.value.w);
                    }
                    */
                    /*
                    for(const auto &v : boneScaleVector)
                    {
                        printf("time: %f, scale: [%f, %f, %f]\n",
                            v.timeStamp, v.value.x, v.value.y, v.value.z);
                    }
                    */
                }
                outModel.bones[boneIndex].childrenIndices.reserve(32);
                for (uint32_t index : data.nodes[jointIndex].childNodeIndices)
                {
                    const auto& joints = data.skins[0].joints;

                    for (uint32_t ind = 0u; ind < joints.size(); ++ind)
                    {
                        if (joints[ind] == index)
                        {
                            outModel.bones[boneIndex].childrenIndices.push_back(ind);
                            break;
                        }
                    }
                }

            }

            float animStartTime = 1e10;
            float animEndTime = -1e10;
            for (uint32_t animationIndex = 0; animationIndex < data.animationNodes.getSize(); ++animationIndex)
            {
                for (const auto& bones : outModel.animationPosData[animationIndex])
                {
                    for (const auto& anim : bones)
                    {
                        animStartTime = std::min(animStartTime, anim.timeStamp);
                        animEndTime = std::max(animEndTime, anim.timeStamp);
                    }
                }

                for (const auto& bones : outModel.animationRotData[animationIndex])
                {
                    for (const auto& anim : bones)
                    {
                        animStartTime = std::min(animStartTime, anim.timeStamp);
                        animEndTime = std::max(animEndTime, anim.timeStamp);
                    }
                }

                for (const auto& bones : outModel.animationScaleData[animationIndex])
                {
                    for (const auto& anim : bones)
                    {
                        animStartTime = std::min(animStartTime, anim.timeStamp);
                        animEndTime = std::max(animEndTime, anim.timeStamp);
                    }
                }
                outModel.animStartTimes[animationIndex] = animStartTime;
                outModel.animEndTimes[animationIndex] = animEndTime;
            }
        }
    }
/*
    const ArraySliceView<GltfModel::Vertex> vertices(&outModel.vertices[0], outModel.vertices.size());
    for(uint32_t i = 0; i < vertices.size(); ++i)
    {
        printf("pos vert: %i:   x: %f, y: %f, z: %f\n", i, vertices[i].pos.x, vertices[i].pos.y, vertices[i].pos.z);
        printf("nor vert: %i:   x: %f, y: %f, z: %f\n", i, vertices[i].norm.x, vertices[i].norm.y, vertices[i].norm.z);
        printf("col vert: %i:   x: %f, y: %f, z: %f, w: %f\n", i, vertices[i].color.x, vertices[i].color.y, vertices[i].color.z, vertices[i].color.w);
    }

    const ArraySliceView<GltfModel::AnimationVertex> animationVertices(
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
*/
    return true;
}

static bool evaluateBone(const GltfModel &model, uint32_t animationIndex, uint32_t boneIndex,
    float time, const Matrix &parentMatrix, PodVector<Matrix> &outMatrices)
{
    if(boneIndex >= model.bones.size())
        return false;

    Vec3 pos;
    Quat rot;
    Vec3 scale;

    auto getInterpolationValue = [](float prevTime, float nextTime, float currTime)
    {
        currTime = std::max(prevTime, currTime);
        currTime = std::min(nextTime, currTime);
        float duration = (nextTime - prevTime);

        float frac = duration > 0.0f ? (currTime - prevTime) / duration : 1.0f;
        return frac;
    };

    const PodVector<GltfModel::BoneAnimationPosOrScale> &posTime = model.animationPosData[animationIndex][boneIndex];
    if(posTime.size() > 0)
    {
        pos = posTime[0].value;
    }
    for(uint32_t nextI = 1; nextI < posTime.size(); ++nextI)
    {
        const GltfModel::BoneAnimationPosOrScale &prev = posTime[nextI - 1];
        const GltfModel::BoneAnimationPosOrScale &next = posTime[nextI];
        if((time <= next.timeStamp) || nextI == posTime.size() - 1)
        {
            float frac = getInterpolationValue(prev.timeStamp, next.timeStamp, time);
            pos = prev.value * (1.0f - frac) + next.value * frac;
            break;
        }
    }

    const PodVector<GltfModel::BoneAnimationRot> &rotTime = model.animationRotData[animationIndex][boneIndex];
    for(uint32_t i = 0; i < rotTime.size(); ++i)
    {
        uint32_t nextI = std::min(i + 1, rotTime.size() - 1);
        const GltfModel::BoneAnimationRot &prev = rotTime[i];
        const GltfModel::BoneAnimationRot &next = rotTime[nextI];
        if((time <= next.timeStamp) || i == nextI)
        {
            float frac = getInterpolationValue(prev.timeStamp, next.timeStamp, time);
            rot = normalize(lerp(prev.value, next.value, frac));
            break;
        }
    }

    const PodVector<GltfModel::BoneAnimationPosOrScale> &scaleTime = model.animationScaleData[animationIndex][boneIndex];
    for(uint32_t i = 0; i < scaleTime.size(); ++i)
    {
        uint32_t nextI = std::min(i + 1, scaleTime.size() - 1);
        const GltfModel::BoneAnimationPosOrScale &prev = scaleTime[i];
        const GltfModel::BoneAnimationPosOrScale &next = scaleTime[nextI];
        if((time <= next.timeStamp) || i == nextI)
        {
            float frac = getInterpolationValue(prev.timeStamp, next.timeStamp, time);
            scale = prev.value * (1.0f - frac) + next.value * frac;
            break;
        }
    }
    
    
    Matrix res = getModelMatrix({pos, rot, scale});
    Matrix newParent = parentMatrix * res;

    outMatrices[boneIndex] =  newParent * model.inverseMatrices[boneIndex];
    for(uint32_t childIndex : model.bones[boneIndex].childrenIndices)
    {
        bool success = evaluateBone(model, animationIndex, childIndex, time, newParent, outMatrices);
        if(!success)
            return false;
    }
    return true;
}

bool evaluateAnimation(const GltfModel &model, uint32_t animationIndex, float time,
    PodVector<Matrix> &outMatrices)
{
    if(model.bones.size() == 0)
        return false;

    animationIndex = animationIndex < model.animationPosData.size() ? animationIndex : model.animationPosData.size() - 1;

    if(animationIndex >= model.animStartTimes.size())
        return false;

    float animStartTime = model.animStartTimes[animationIndex];
    float animEndTime = model.animEndTimes[animationIndex];

    while (time < animStartTime)
        time += animEndTime - animStartTime;
    while (time > animEndTime)
        time -= animEndTime - animStartTime;

    if (model.animationPosData.size() > 255)
        return false;
    outMatrices.uninitializedResize(256);
    return evaluateBone(model, animationIndex, 0, time, Matrix(), outMatrices);
}
