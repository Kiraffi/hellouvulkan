#include "components.h"

#include "core/assert.h"
#include "core/general.h"
#include "core/log.h"

#include <unordered_set>

#undef SER_DATA_BEGIN
#undef SER_DATA_END

// For debugging that we have all types set, and all types are unique indices
static std::unordered_set<uint32_t> AllDataTypes;


static void printFieldValue(const char* fieldName, void *value, FieldType field);
static bool writeFieldInfo(const FieldInfo &info, WriteJson &writeJson);

static bool parseFieldInfo(const FieldInfo &info, const JsonBlock &json)
{
    switch(info.type)
    {
        case FieldType::IntType:
        {
            int &i = *((int*)info.fieldMemoryAddress);
            if(!json.getChild(info.fieldName).parseInt(i))
                return false;
            break;
        }
        case FieldType::FloatType:
        {
            float &f = *((float*)info.fieldMemoryAddress);
            if(!json.getChild(info.fieldName).parseFloat(f))
                return false;
            break;
        }
        case FieldType::Vec2Type:
        {
            Vector2 &v = *((Vector2*)info.fieldMemoryAddress);
            if(!json.getChild(info.fieldName).parseVec2(v))
                return false;
            break;
        }
        case FieldType::Vec3Type:
        {
            Vector3 &v = *((Vector3*)info.fieldMemoryAddress);
            if(!json.getChild(info.fieldName).parseVec3(v))
                return false;
            break;
        }
        case FieldType::Vec4Type:
        {
            Vector4 &v = *((Vector4*)info.fieldMemoryAddress);
            if(!json.getChild(info.fieldName).parseVec4(v))
                return false;
            break;
        }


        default:
        {
            ASSERT_STRING(false, "Unknown field type!");
        }

    }
    return true;
}

static bool writeFieldInfo(const FieldInfo &info, WriteJson &writeJson)
{
    if(!info.isValid)
        return false;

    //LOG("Field: %s, index: %i, type: %i, ", info.fieldName, info.fieldIndex, info.type);
    printFieldValue(info.fieldName, info.fieldMemoryAddress, info.type); \
    switch(info.type)
    {
        case FieldType::IntType:
        {
            int i = *((int*)info.fieldMemoryAddress);
            writeJson.addInteger(info.fieldName, i);
            break;
        }
        case FieldType::FloatType:
        {
            float f = *((float*)info.fieldMemoryAddress);
            writeJson.addNumber(info.fieldName, f);
            break;
        }
        case FieldType::Vec2Type:
        {
            const Vector2 &v = *((Vector2*)info.fieldMemoryAddress);
            writeJson.writeVec2(info.fieldName, v);
            break;
        }
        case FieldType::Vec3Type:
        {
            const Vector3 &v = *((Vector3*)info.fieldMemoryAddress);
            writeJson.writeVec3(info.fieldName, v);
            break;
        }
        case FieldType::Vec4Type:
        {
            const Vector4 &v = *((Vector4*)info.fieldMemoryAddress);
            writeJson.writeVec4(info.fieldName, v);
            break;
        }

        default:
        {
            ASSERT_STRING(false, "Unknown field type!");
        }
    }

    return writeJson.isValid();
}


static void printFieldValue(const char* fieldName, void *value, FieldType field)
{
    if(!value)
    {
        return;
    }

    switch(field)
    {
        case IntType:
        {
            LOG("%s: %i\n", fieldName, *((int *)value));
            break;
        }
        case FloatType:
        {
            LOG("%s: %f\n", fieldName, *((float *)value));
            break;
        }

        case Vec2Type:
        {
            const Vector2 &v = *((Vector2 *)value);
            LOG("%s: [x: %f, y: %f]\n", fieldName, v.x, v.y);
            break;
        }

        case Vec3Type:
        {
            const Vector3 &v = *((Vector3 *)value);
            LOG("%s: [x: %f, y: %f, z: %f]\n", fieldName, v.x, v.y, v.z);
            break;
        }

        case Vec4Type:
        {
            const Vector4 &v = *((Vector4 *)value);
            LOG("%s: [x: %f, y: %f, z: %f, w: %f]\n", fieldName, v.x, v.y, v.z, v.w);
            break;
        }

        default:
        {
            break;
        }
    }
}
// Checker to see that every data class is unique
#define SER_DATA_BEGIN_HELPER_CHECK(CLASS_NAME, MAGIC_NUMBER) \
struct RunOnce##CLASS_NAME \
{ \
    RunOnce##CLASS_NAME() \
    { \
        if(AllDataTypes.find(MAGIC_NUMBER) != AllDataTypes.end()) \
        { \
            ASSERT_STRING(false, #MAGIC_NUMBER " added already"); \
        } \
        AllDataTypes.insert(MAGIC_NUMBER); \
    } \
}; \
static RunOnce##CLASS_NAME runOnce##CLASS_NAME; \

#define SER_DATA_BEGIN(CLASS_NAME, MAGIC_NUMBER, VERSION_NUMBER) \
\
SER_DATA_BEGIN_HELPER_CHECK(CLASS_NAME, MAGIC_NUMBER) \
\
bool CLASS_NAME::isJsonType(const JsonBlock &json) \
{ \
    if(!json.isObject()) \
        return false; \
\
    if(!json.getChild("ComponentType").equals(#CLASS_NAME)) \
        return false; \
    if(!json.getChild("ComponentTypeId").equals(MAGIC_NUMBER)) \
        return false; \
\
    return true; \
} \
bool CLASS_NAME::serialize(WriteJson &writeJson) \
{ \
    writeJson.addObject(); \
    writeJson.addString("ComponentType", #CLASS_NAME); \
    writeJson.addInteger("ComponentTypeId", MAGIC_NUMBER); \
    writeJson.addInteger("ComponentVersion", VERSION_NUMBER); \
    for(int i = 0; i < CLASS_NAME::getFieldCount(); ++i) \
    { \
        FieldInfo info = getFieldInfoIndex(i); \
        if(!writeFieldInfo(info, writeJson)) \
            return false; \
    } \
    writeJson.endObject(); \
    return writeJson.isValid(); \
} \
void CLASS_NAME::print() const \
{ \
    for(int i = 0; i < CLASS_NAME::getFieldCount(); ++i) \
    { \
        FieldInfo info = getFieldInfoIndex(i); \
        printFieldValue(info.fieldName, info.fieldMemoryAddress, info.type); \
    } \
} \
bool CLASS_NAME::deSerialize(const JsonBlock &json) \
{ \
    if(!CLASS_NAME::isJsonType(json)) \
        return false; \
\
    for(int i = 0; i < CLASS_NAME::getFieldCount(); ++i) \
    { \
        FieldInfo info = getFieldInfoIndex(i); \
        parseFieldInfo(info, json); \
    } \
\
    return true; \
} \
bool CLASS_NAME::getMemoryPtr(const char* fieldName, void **outMemAddress, FieldType &outFieldType) const \
{ \
    FieldInfo info = getFieldInfo(fieldName); \
    *outMemAddress = info.fieldMemoryAddress; \
    outFieldType = info.type; \
    return info.isValid; \
} \
bool CLASS_NAME::setValue(const char* fieldName, void* value, int valueByteSize) \
{ \
    FieldInfo info = getFieldInfo(fieldName); \
    if(value && info.isValid && valueByteSize == info.fieldByteSize && \
        valueByteSize > 0 && info.fieldMemoryAddress) \
    { \
        Supa::memcpy(info.fieldMemoryAddress, value, valueByteSize); \
    } \
    return false; \
} \
FieldInfo CLASS_NAME::getFieldInfo(const char *fieldName) const \
{ \
    CLASS_NAME::HelperStruct fieldInfos = getFieldInfos(); \
    for(int i = 0; i < CLASS_NAME::getFieldCount(); ++i) \
    { \
        if(Supa::strcmp(fieldName, fieldInfos.arr[i].fieldName) == 0) \
            return fieldInfos.arr[i]; \
    } \
    return FieldInfo{}; \
} \
FieldInfo CLASS_NAME::getFieldInfoIndex(int fieldIndex) const \
{ \
    if(fieldIndex < 0 || fieldIndex >= CLASS_NAME::getFieldCount()) \
        return FieldInfo{}; \
    return getFieldInfos().arr[fieldIndex]; \
} \
CLASS_NAME::HelperStruct CLASS_NAME::getFieldInfos() const \
{ \
    return CLASS_NAME::HelperStruct { \
        .arr = {

#define SER_DATA_END() \
        } \
    }; \
}


#undef HELPER_FIELD
#undef HELPER_FIELD_COPY_TYPE
#undef HELPER_FIELD_NO_COPY_TYPE

#define HELPER_FIELD(FIELD_NAME) \
            FieldInfo { \
                .fieldName = get##FIELD_NAME##String(), \
                .fieldMemoryAddress = (void *)&this->FIELD_NAME, \
                .fieldIndex = get##FIELD_NAME##Index(), \
                .fieldByteSize = sizeof(FIELD_NAME), \
                .type = get##FIELD_NAME##Type(), \
                .isValid = true \
            },

#undef FLOAT_FIELD
#undef INT_FIELD
#undef VEC2_FIELD
#undef VEC3_FIELD
#undef VEC4_FIELD

#define FLOAT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) HELPER_FIELD(FIELD_NAME)
#define INT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) HELPER_FIELD(FIELD_NAME)

#define VEC2_FIELD(FIELD_NAME, DEF_VAL1, DEF_VAL2)   HELPER_FIELD(FIELD_NAME)
#define VEC3_FIELD(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3)   HELPER_FIELD(FIELD_NAME)
#define VEC4_FIELD(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, DEF_VAL4)   HELPER_FIELD(FIELD_NAME)

// Make sure include works
#ifdef COMPONENTS_INCLUDE_FILE_H
    #undef COMPONENTS_INCLUDE_FILE_H
#endif

#include "components.h"
