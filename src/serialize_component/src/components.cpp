#include "serialize.h"
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
\
bool CLASS_NAME::deSerialize() \
{ \
    return true; \
} \
\
bool CLASS_NAME::getMemoryPtr(const char* fieldName, void **outMemAddress, FieldType &outFieldType) \
{ \
    FieldInfo info = getFieldInfo(fieldName); \
    *outMemAddress = info.fieldMemoryAddress; \
    outFieldType = info.type; \
    return info.isValid; \
} \
\
FieldInfo CLASS_NAME::getFieldInfo(const char *fieldName) const \
{ \

#define SER_DATA_END() \
    return FieldInfo{}; \
}



#undef HELPER_FIELD
#undef HELPER_FIELD_COPY_TYPE
#undef HELPER_FIELD_NO_COPY_TYPE

#define HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) \
    if(Supa::strcmp(fieldName, FIELD_NAME##String) == 0) \
    { \
        return FieldInfo { \
            .fieldName = get##FIELD_NAME##String(), \
            .fieldMemoryAddress = (void *)&this->FIELD_NAME, \
            .fieldIndex = get##FIELD_NAME##Index(), \
            .type = get##FIELD_NAME##Type(), \
            .isValid = true \
        }; \
    }


#undef FLOAT_FIELD
#undef INT_FIELD

#define FLOAT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE)
#define INT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE)

// Make sure include works
#ifdef COMPONENTS_INCLUDE_FILE_H
    #undef COMPONENTS_INCLUDE_FILE_H
#endif

#include "components.h"










#undef SER_DATA_BEGIN
#undef SER_DATA_END

#define SER_DATA_BEGIN(CLASS_NAME, MAGIC_NUMBER, VERSION_NUMBER) \
FieldInfo CLASS_NAME::getFieldInfoIndex(int fieldIndex) const \
{ \
    switch(fieldIndex) \
    { \
    
#define SER_DATA_END() \
        default: \
        { \
            break; \
        } \
    } \
    return FieldInfo{}; \
}


#undef HELPER_FIELD
#undef HELPER_FIELD_COPY_TYPE
#undef HELPER_FIELD_NO_COPY_TYPE


#define HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) \
    case get##FIELD_NAME##Index(): \
    { \
        return FieldInfo { \
            .fieldName = get##FIELD_NAME##String(), \
            .fieldMemoryAddress = (void *)&this->FIELD_NAME, \
            .fieldIndex = get##FIELD_NAME##Index(), \
            .type = get##FIELD_NAME##Type(), \
            .isValid = true \
        }; \
    }

#undef FLOAT_FIELD
#undef INT_FIELD

#define FLOAT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE)
#define INT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE)

// Make sure include works
#ifdef COMPONENTS_INCLUDE_FILE_H
    #undef COMPONENTS_INCLUDE_FILE_H
#endif
#include "components.h"
