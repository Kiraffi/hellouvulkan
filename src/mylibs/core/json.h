#pragma once

#include "mytypes.h"

#include <container/string.h>
#include <container/stringview.h>
#include <container/vector.h>

#include <math/quaternion.h>
#include <math/vector3.h>

template<typename T>
class PodVector;

class JsonBlock
{
public:
    enum ValueTypes : int32_t
    {
        VALID_TYPE = 1 << 0,
        ARRAY_TYPE = 1 << 1,
        OBJECT_TYPE = 1 << 2,
        STRING_TYPE = 1 << 3,
        INT_TYPE = 1 << 4,
        DOUBLE_TYPE = 1 << 5,
        BOOL_TYPE = 1 << 6
    };

    bool parseJson(const StringView &data);

    bool isValid() const { return ( jType & VALID_TYPE ) == VALID_TYPE; }
    bool isArray() const { return jType == (VALID_TYPE | ARRAY_TYPE); }
    bool isObject() const { return jType == (VALID_TYPE | OBJECT_TYPE); }
    bool isString() const { return jType == (VALID_TYPE | STRING_TYPE); }
    bool isInt() const { return jType == (VALID_TYPE | INT_TYPE); }
    bool isDouble() const { return jType == (VALID_TYPE | DOUBLE_TYPE); }
    bool isBool() const { return jType == (VALID_TYPE | BOOL_TYPE); }

    bool parseString(StringView &outString) const;
    bool parseDouble(double &outDouble) const;
    bool parseFloat(float &outFloat) const;
    bool parseInt(int64_t &outInt) const;
    bool parseInt(int &outInt) const;
    bool parseUInt(uint32_t &outInt) const;

    bool parseNumber(double& outDouble) const;
    bool parseNumber(float& outFloat) const;

    bool parseBool(bool &outBool) const;
    bool parseBuffer(PodVector<uint8_t> &outBuffer) const;

    bool parseVec2(Vector2 &v) const;
    bool parseVec3(Vector3 &v) const;
    bool parseVec4(Vector4 &v) const;
    bool parseQuat(Quaternion &q) const;

    bool parseNumberArray(double *arr, uint32_t arrayLen) const;
    bool parseNumberArray(float *arr, uint32_t arrayLen) const;
    bool parseIntegerArray(int64_t *arr, uint32_t arrayLen) const;
    bool parseIntegerArray(int32_t *arr, uint32_t arrayLen) const;

    bool equals(uint32_t value) const;
    bool equals(StringView str) const;

    bool hasChild(StringView childName) const;

    int getChildCount() const { return ( int )children.size(); }
    const JsonBlock &getChild(int index) const;
    const JsonBlock &getChild(StringView childName) const;

    const JsonBlock *const begin() const;
    const JsonBlock *const end() const;

    bool print() const;

    Vector< JsonBlock > children;
    StringView blockName;

    bool named = false;

    bool valueBool = false;
    StringView valueStr;
    double valueDbl = -1.0;
    int64_t valueInt = -1;

    int jType = 0;

    static const JsonBlock emptyBlock;
};
