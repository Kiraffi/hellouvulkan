#pragma once

#include <core/json.h>
#include <core/writejson.h>


// Never delete any value, only add to this...
// Might be bit bad if multiple people were to modify at the same time or at least possibly cause problems.
enum class ComponentType : u32
{
    HeritagedType = 1,
    HeritagedType2,
    HeritagedType3,


    ComponentTypeCount
};

enum class EntityType : u16
{
    StaticModelEntityType = 1,


    EntityTypeCount
};

struct EntitySystemHandle
{
    EntityType entitySystemIndex = EntityType::EntityTypeCount;
    u16 entityIndexVersion = ~u16(0);
    u32 entityIndex = ~u32(0);
 };


enum class FieldType
{
    IntType,
    FloatType,


    Vec2Type,
    Vec3Type,
    Vec4Type,

    NumTypes
};


bool serializeField(WriteJson &writeJson,
    const char* const fieldName,
    const void* const fieldMemoryAddress,
    FieldType fieldType);

bool deserializeField(const JsonBlock &json,
    const char* const fieldName,
    void* fieldMemoryAddress,
    FieldType fieldType);



struct FieldInfo
{
    const char* fieldName;
    void* fieldMemoryAddress;
    int fieldIndex;
    int fieldByteSize;
    FieldType type;
    bool isValid;
};

/*
class SerializableClassBase
{
public:
    //SerializableClassBase() : objMagicNumber(magicNumberClass), objVersion(versionClass) {}
    //virtual ~SerializableClassBase() {}

    virtual bool serialize() { return true; };
    virtual bool deSerialize() { return true; };

    // Needed maybe, maybe not?
    static constexpr int getFileMagicNumber() { return 0xABBAABB; }
    virtual const int getClassMagicNumber() const { return SerializableClassBase::magicNumberClass; }
    virtual const int getClassVersion() const { return SerializableClassBase::versionClass; }

    static constexpr int getStaticClassMagicNumber() { return SerializableClassBase::magicNumberClass; }
    static constexpr int getStaticClassVersion() { return SerializableClassBase::versionClass; }

    int getObjMagicNumber() const { return objMagicNumber; }
    int getObjVersion() const { return objVersion; }

protected:
    int objMagicNumber;
    int objVersion;

private:
    static constexpr int magicNumberClass = 11;
    static constexpr int versionClass = 1;

};
*/
struct SerializableClassBase
{
    int getObjMagicNumber() const { return objMagicNumber; }
    int getObjVersion() const { return objVersion; }
    int getObjSize() const { return objSize; }
    int objMagicNumber;
    int objVersion;
    int objSize;
};
// remove inheritage

#define SER_DATA_BEGIN(CLASS_NAME, MAGIC_NUMBER, VERSION_NUMBER) \
class CLASS_NAME \
{ \
private: \
    struct HelperStruct; \
public: \
    /* CLASS_NAME() { serObjBase.objMagicNumber = magicNumberClass; serObjBase.objVersion = versionClass; serObjBase.objSize = sizeof(CLASS_NAME); } */ \
    static constexpr int getFileMagicNumber() { return 0xABBAABB; } \
    static constexpr int getStaticClassMagicNumber() { return MAGIC_NUMBER; } \
    static constexpr int getStaticClassVersion() { return VERSION_NUMBER; } \
\
    static bool isJsonType(const JsonBlock &json); \
\
    SerializableClassBase getBase() const { \
        return SerializableClassBase { \
            .objMagicNumber = MAGIC_NUMBER, .objVersion = VERSION_NUMBER, .objSize = sizeof(CLASS_NAME) \
        }; \
    } \
    /* SerializableClassBase &getBase() { return serObjBase; } */ \
    /* int getObjMagicNumber() const { return serObjBase.getObjMagicNumber(); } */ \
    /* int getObjVersion() const { return serObjBase.getObjVersion(); } */ \
    /* int getObjSize() const { return serObjBase.getObjSize(); } */ \
\
    void print() const; \
    bool setValue(const char* fieldName, void* value, int valueByteSize); \
    bool getMemoryPtr(const char* fieldName, void **outMemAddress, FieldType &outFieldType) const; \
    void* getMemory(const char* fieldName) const ; \
    bool serialize(WriteJson &writeJson); \
    bool deSerialize(const JsonBlock &json); \
private: \
    static constexpr int getStartParameterRow() { return __LINE__; } \
    HelperStruct getFieldInfos() const;  \
    FieldInfo getFieldInfo(const char* fieldName) const; \
    FieldInfo getFieldInfoIndex(int index) const; \
    /* static constexpr int magicNumberClass = MAGIC_NUMBER; */ \
    /* static constexpr int versionClass = VERSION_NUMBER; */ \
    static constexpr int startParameterRow = __LINE__; \
\
    //SerializableClassBase serObjBase;

#define SER_DATA_END(CLASS_NAME) \
private: \
    static constexpr const int getEndParameterRow() { return __LINE__; } \
    static const int getFieldCount() { return __LINE__ - 1 - CLASS_NAME::getStartParameterRow(); } \
    /* static constexpr int endParameterRow = __LINE__; */ \
    static constexpr int fieldCount = __LINE__ - 1 - startParameterRow; \
    \
    struct HelperStruct \
    { \
        FieldInfo arr[ fieldCount ]; \
    }; \
};





#define HELPER_FIELD(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
public: \
    FIELD_TYPE& get##FIELD_NAME() { return FIELD_NAME; } \
    static constexpr const char* get##FIELD_NAME##String() { return #FIELD_NAME; } \
    static constexpr int get##FIELD_NAME##Index() { return /*FIELD_NAME##Index;*/ __LINE__ - getStartParameterRow() - 1; } \
    static constexpr FieldType get##FIELD_NAME##Type() { return FIELD_TYPE_TYPE; } \
    /*static constexpr const char* FIELD_NAME##String = #FIELD_NAME;*/ \
private: \
//    static constexpr int FIELD_NAME##Index = __LINE__ - startParameterRow - 1;

#define HELPER_FIELD1(FIELD_NAME, FIELD_DEFAULT_VALUE, FIELD_TYPE) \
    protected: \
        FIELD_TYPE FIELD_NAME = FIELD_DEFAULT_VALUE; \


#define HELPER_FIELD2(FIELD_NAME, DEFAULT_VAL1, DEFAULT_VAL2, FIELD_TYPE) \
    protected: \
        FIELD_TYPE FIELD_NAME = {DEFAULT_VAL1, DEFAULT_VAL2}; \

#define HELPER_FIELD3(FIELD_NAME, DEFAULT_VAL1, DEFAULT_VAL2, DEFAULT_VAL3, FIELD_TYPE) \
    protected: \
        FIELD_TYPE FIELD_NAME = {DEFAULT_VAL1, DEFAULT_VAL2, DEFAULT_VAL3}; \

#define HELPER_FIELD4(FIELD_NAME, DEFAULT_VAL1, DEFAULT_VAL2, DEFAULT_VAL3, DEFAULT_VAL4, FIELD_TYPE) \
    protected: \
        FIELD_TYPE FIELD_NAME = {DEFAULT_VAL1, DEFAULT_VAL2, DEFAULT_VAL3, DEFAULT_VAL4}; \


// Setting and getting simple copy types like int, float etc....
#define HELPER_FIELD_COPY_TYPE(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
public: \
    FIELD_TYPE get##FIELD_NAME() const { return FIELD_NAME; } \
    bool set##FIELD_NAME(FIELD_TYPE value) { FIELD_NAME = value; return true; }



// Setting and getting ref types, mostly for structs and classes....
#define HELPER_FIELD_NO_COPY_TYPE(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
public: \
    const FIELD_TYPE &get##FIELD_NAME() const { return FIELD_NAME; } \
    bool set##FIELD_NAME(const FIELD_TYPE &value) { FIELD_NAME = value; return true; }








#define HELPER_FIELD_COPY_TYPE1(FIELD_NAME, DEF_VAL1, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD_COPY_TYPE(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD1(FIELD_NAME, DEF_VAL1, FIELD_TYPE)

#define HELPER_FIELD_COPY_TYPE2(FIELD_NAME, DEF_VAL1, DEF_VAL2, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD_COPY_TYPE(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD2(FIELD_NAME, DEF_VAL1, DEF_VAL2, FIELD_TYPE)

#define HELPER_FIELD_COPY_TYPE3(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD_COPY_TYPE(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD3(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, FIELD_TYPE)

#define HELPER_FIELD_COPY_TYPE4(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, DEF_VAL4, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD_COPY_TYPE(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD4(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, DEF_VAL4, FIELD_TYPE)


#define HELPER_FIELD_NO_COPY_TYPE1(FIELD_NAME, DEF_VAL1, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD_NO_COPY_TYPE(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD1(FIELD_NAME, DEF_VAL1, FIELD_TYPE)


#define HELPER_FIELD_NO_COPY_TYPE2(FIELD_NAME, DEF_VAL1, DEF_VAL2, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD_NO_COPY_TYPE(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD2(FIELD_NAME, DEF_VAL1, DEF_VAL2, FIELD_TYPE)


#define HELPER_FIELD_NO_COPY_TYPE3(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD_NO_COPY_TYPE(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD3(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, FIELD_TYPE)


#define HELPER_FIELD_NO_COPY_TYPE4(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, DEF_VAL4, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD_NO_COPY_TYPE(FIELD_NAME, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD4(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, DEF_VAL4, FIELD_TYPE)









#define FLOAT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) \
    HELPER_FIELD_COPY_TYPE1(FIELD_NAME, FIELD_DEFAULT_VALUE, float, FieldType::FloatType)
#define INT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) \
    HELPER_FIELD_COPY_TYPE1(FIELD_NAME, FIELD_DEFAULT_VALUE, int, FieldType::IntType)

#define VEC2_FIELD(FIELD_NAME, DEF_VAL1, DEF_VAL2 ) \
    HELPER_FIELD_NO_COPY_TYPE2(FIELD_NAME, DEF_VAL1, DEF_VAL2, Vector2, FieldType::Vec2Type)
#define VEC3_FIELD(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3) \
    HELPER_FIELD_NO_COPY_TYPE3(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, Vector3, FieldType::Vec3Type)
#define VEC4_FIELD(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, DEF_VAL4 ) \
    HELPER_FIELD_NO_COPY_TYPE4(FIELD_NAME, DEF_VAL1, DEF_VAL2, DEF_VAL3, DEF_VAL4, Vector4, FieldType::Vec4Type)


