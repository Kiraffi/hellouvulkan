#pragma once

enum FieldType
{
    IntType,
    FloatType,

    NumTypes
};

class SerializableClassBase
{
public:
    SerializableClassBase() { objMagicNumber = magicNumberClass; objVersion = versionClass; }
    virtual ~SerializableClassBase() {}

    virtual bool serialize() { return true; };
    virtual bool deSerialize() { return true; };

    // Needed maybe, maybe not?
    static constexpr int getFileMagicNumber() { return 0xABBAABB; }
    static constexpr int getClassMagicNumber() { return magicNumberClass; }
    static constexpr int getClassVersion() { return versionClass; }

    int getObjMagicNumber() const { return objMagicNumber; }
    int getObjVersion() const { return objVersion; }

protected:
    int objMagicNumber;
    int objVersion;

private:
    static constexpr int magicNumberClass = 11;
    static constexpr int versionClass = 1;

};


#define SER_DATA_BEGIN(CLASS_NAME, MAGIC_NUMBER, VERSION_NUMBER) \
class CLASS_NAME : public SerializableClassBase \
{ \
public: \
    CLASS_NAME() : SerializableClassBase() { objMagicNumber = magicNumberClass; objVersion = versionClass; } \
    static constexpr int getClassMagicNumber() { return magicNumberClass; } \
    static constexpr int getClassVersion() { return versionClass; } \
    bool getMemoryPtr(const char* fieldName, void **outMemAddress, FieldType &outFieldType); \
    virtual bool serialize() override; \
    virtual bool deSerialize() override; \
private: \
    static constexpr int magicNumberClass = MAGIC_NUMBER; \
    static constexpr int versionClass = VERSION_NUMBER; \
    static constexpr int startParameterRow = __LINE__;

#define SER_DATA_END() \
private: \
    static constexpr int endParameterRow = __LINE__; \
    static constexpr int fieldCount = __LINE__ - 1 - startParameterRow; \
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
    static constexpr int FIELD_NAME##Index = __LINE__ - startParameterRow - 1; \



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



