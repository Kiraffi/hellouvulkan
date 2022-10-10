#include "components.h"

#include "core/general.h"
#include "core/log.h"


#undef SER_DATA_BEGIN
#undef SER_DATA_END


static void printFieldValue(void *value, FieldType field)
{
    if(!value)
    {
        return;
    }

    switch(field)
    {
        case IntType:
        {
            LOG("value: %i\n", *((int *)value));
            break;
        }
        case FloatType:
        {
            LOG("value: %f\n", *((float *)value));
            break;
        }

        default:
        {
            break;
        }
    }
}

#define SER_DATA_BEGIN(CLASS_NAME, MAGIC_NUMBER, VERSION_NUMBER) \
bool CLASS_NAME::serialize() \
{ \
    for(int i = 0; i < CLASS_NAME::fieldCount; ++i) \
    { \
        FieldInfo info = getFieldInfoIndex(i); \
        if(info.isValid) \
        { \
            LOG("Field: %s, index: %i, type: %i, ", info.fieldName, info.fieldIndex, info.type); \
            printFieldValue(info.fieldMemoryAddress, info.type); \
        } \
    } \
    return true; \
} \
bool CLASS_NAME::deSerialize() \
{ \
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
    for(int i = 0; i < CLASS_NAME::fieldCount; ++i) \
    { \
        if(Supa::strcmp(fieldName, fieldInfos.arr[i].fieldName) == 0) \
            return fieldInfos.arr[i]; \
    } \
    return FieldInfo{}; \
} \
FieldInfo CLASS_NAME::getFieldInfoIndex(int fieldIndex) const \
{ \
    if(fieldIndex < 0 || fieldIndex >= CLASS_NAME::fieldCount) \
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

#define HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) \
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

#define FLOAT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE)
#define INT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE)

// Make sure include works
#ifdef COMPONENTS_INCLUDE_FILE_H
    #undef COMPONENTS_INCLUDE_FILE_H
#endif

#include "components.h"
