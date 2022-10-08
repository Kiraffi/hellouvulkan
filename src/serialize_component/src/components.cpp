#include "serialize.h"
#include "components.h"

#include "core/general.h"
#include "core/log.h"
#if 1
#undef SER_CLASS_BEGIN
#undef INT_FIELD
#undef FLOAT_FIELD
#undef SER_CLASS_END
#undef HELPER_FIELD
#undef HELPER_FIELD_COPY_TYPE
#undef HELPER_FIELD_NO_COPY_TYPE






#define SER_DATA_BEGIN(CLASS_NAME, MAGIC_NUMBER, VERSION_NUMBER) \
/* static constexpr const char* getFieldStringFromClass##CLASS_NAME(int index); */ \
bool CLASS_NAME::serialize() \
{ \
    /* for(int i = 0; i < CLASS_NAME::fieldCount; ++i) */ \
    /* {  */ \
    /*     LOG("Field: %s\n", getFieldStringFromClass##CLASS_NAME(i)); */ \
    /* } */ \
    return true; \
} \
bool CLASS_NAME::deSerialize() \
{ \
    return true; \
} \
bool CLASS_NAME::getMemoryPtr(const char* fieldName, void **outMemAddress, FieldType &outFieldType) \
{ \
    *outMemAddress = nullptr; \
    outFieldType = FieldType::NumTypes; \
    for(int i = 0; i < CLASS_NAME::fieldCount; ++i) \
    { \
/*
        if(Supa::strcmp(fieldName, getFieldStringFromClass##CLASS_NAME(i)) == 0) \
        { \
            *outMemAddress = &this->FIELD_NAME; \
            outFieldType = get##FIELD_NAME##Type(); \
            return true; \
        } \
    } \
    return false; \
} \
*/



#define SER_DATA_END() \
    } \
    return false; \
} \

 
#define HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) \
    if(Supa::strcmp(fieldName, FIELD_NAME##String) == 0) \
    { \
        *outMemAddress = &this->FIELD_NAME; \
        outFieldType = get##FIELD_NAME##Type(); \
        return true; \
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
#endif