#pragma once

#include <core/json.h>
#include <core/writejson.h>

enum FieldType
{
    IntType,
    FloatType,

    NumTypes
};


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
    static constexpr int getStaticClassMagicNumber() { return CLASS_NAME::magicNumberClass; } \
    static constexpr int getStaticClassVersion() { return CLASS_NAME::versionClass; } \
\
    static bool isJsonType(const JsonBlock &json); \
\
    SerializableClassBase getBase() const { \
        return SerializableClassBase { \
            .objMagicNumber = CLASS_NAME::magicNumberClass, .objVersion = CLASS_NAME::versionClass, .objSize = sizeof(CLASS_NAME) \
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
    HelperStruct getFieldInfos() const;  \
    FieldInfo getFieldInfo(const char* fieldName) const; \
    FieldInfo getFieldInfoIndex(int index) const; \
    static constexpr int magicNumberClass = MAGIC_NUMBER; \
    static constexpr int versionClass = VERSION_NUMBER; \
    static constexpr int startParameterRow = __LINE__; \
\
    //SerializableClassBase serObjBase;

#define SER_DATA_END() \
private: \
    static constexpr int endParameterRow = __LINE__; \
    static constexpr int fieldCount = __LINE__ - 1 - startParameterRow; \
    \
    struct HelperStruct \
    { \
        FieldInfo arr[fieldCount];\
    }; \
};





#define HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE, FIELD_TYPE, FIELD_TYPE_TYPE) \
public: \
    FIELD_TYPE& get##FIELD_NAME() { return FIELD_NAME; } \
    static constexpr const char* get##FIELD_NAME##String() { return FIELD_NAME##String; } \
    static constexpr int get##FIELD_NAME##Index() { return FIELD_NAME##Index; } \
    static constexpr FieldType get##FIELD_NAME##Type() { return FIELD_TYPE_TYPE; } \
protected: \
    FIELD_TYPE FIELD_NAME = FIELD_DEFAULT_VALUE; \
    static constexpr const char* FIELD_NAME##String = #FIELD_NAME; \
private: \
    static constexpr int FIELD_NAME##Index = __LINE__ - startParameterRow - 1;



// Setting and getting simple copy types like int, float etc....
#define HELPER_FIELD_COPY_TYPE(FIELD_NAME, FIELD_DEFAULT_VALUE, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE, FIELD_TYPE, FIELD_TYPE_TYPE) \
public: \
    FIELD_TYPE get##FIELD_NAME() const { return FIELD_NAME; } \
    bool set##FIELD_NAME(FIELD_TYPE value) { FIELD_NAME = value; return true; }



// Setting and getting ref types, mostly for structs and classes....
#define HELPER_FIELD_NO_COPY_TYPE(FIELD_NAME, FIELD_DEFAULT_VALUE, FIELD_TYPE, FIELD_TYPE_TYPE) \
    HELPER_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE, FIELD_TYPE, FIELD_TYPE_TYPE) \
public: \
    FIELD_TYPE &get##FIELD_NAME() const { return FIELD_NAME; } \
    bool set##FIELD_NAME(const FIELD_TYPE &value) { FIELD_NAME = value; return true; }




#define FLOAT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE) HELPER_FIELD_COPY_TYPE(FIELD_NAME, FIELD_DEFAULT_VALUE, float, FieldType::FloatType)
#define INT_FIELD(FIELD_NAME, FIELD_DEFAULT_VALUE)   HELPER_FIELD_COPY_TYPE(FIELD_NAME, FIELD_DEFAULT_VALUE, int, FieldType::IntType)



