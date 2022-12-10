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
    enum ValueTypes : i32
    {
        VALID_TYPE =  1 << 0,
        ARRAY_TYPE =  1 << 1,
        OBJECT_TYPE = 1 << 2,
        STRING_TYPE = 1 << 3,
        INT_TYPE =    1 << 4,
        DOUBLE_TYPE = 1 << 5,
        BOOL_TYPE =   1 << 6
    };

    bool parseJson(const StringView &data);

    bool isValid()  const { return ( jType & VALID_TYPE ) == VALID_TYPE; }
    bool isArray()  const { return jType == (VALID_TYPE | ARRAY_TYPE);   }
    bool isObject() const { return jType == (VALID_TYPE | OBJECT_TYPE);  }
    bool isString() const { return jType == (VALID_TYPE | STRING_TYPE);  }
    bool isInt()    const { return jType == (VALID_TYPE | INT_TYPE);     }
    bool isDouble() const { return jType == (VALID_TYPE | DOUBLE_TYPE);  }
    bool isBool()   const { return jType == (VALID_TYPE | BOOL_TYPE);    }

    bool parseString(StringView &outString) const;
    bool parseDouble(double &outDouble) const;
    bool parseFloat(float &outFloat) const;
    bool parseInt(i64 &outInt) const;
    bool parseInt(i32 &outInt) const;
    bool parseUInt(u32 &outInt) const;

    bool parseNumber(double& outDouble) const;
    bool parseNumber(float& outFloat) const;

    bool parseBool(bool &outBool) const;
    bool parseBuffer(PodVector<u8> &outBuffer) const;

    bool parseVec2(Vector2 &v) const;
    bool parseVec3(Vector3 &v) const;
    bool parseVec4(Vector4 &v) const;
    bool parseQuat(Quaternion &q) const;

    bool parseNumberArray(double *arr, u32 arrayLen) const;
    bool parseNumberArray(float *arr, u32 arrayLen) const;
    bool parseIntegerArray(i64 *arr, u32 arrayLen) const;
    bool parseIntegerArray(i32 *arr, u32 arrayLen) const;

    bool equals(u32 value) const;
    bool equals(StringView str) const;

    bool hasChild(StringView childName) const;

    i32 getChildCount() const { return ( i32 )children.size(); }
    const JsonBlock &getChild(i32 index) const;
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
    i64 valueInt = -1;

    i32 jType = 0;

    static const JsonBlock emptyBlock;
};
