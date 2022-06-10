#pragma once

#include "mytypes.h"

#include <container/arraysliceview.h>
#include <container/podvector.h>
#include <container/vector.h>

#include <string_view>

struct JSONBlock
{
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

    bool parseJSON(const ArraySliceView<char> &data);

    bool isValid() const { return ( jType & VALID_TYPE ) == VALID_TYPE; }
    bool isArray() const { return jType == (VALID_TYPE | ARRAY_TYPE); }
    bool isObject() const { return jType == (VALID_TYPE | OBJECT_TYPE); }
    bool isString() const { return jType == (VALID_TYPE | STRING_TYPE); }
    bool isInt() const { return jType == (VALID_TYPE | INT_TYPE); }
    bool isDouble() const { return jType == (VALID_TYPE | DOUBLE_TYPE); }
    bool isBool() const { return jType == (VALID_TYPE | BOOL_TYPE); }

    bool parseString(std::string_view &outString) const;
    bool parseDouble(double &outDouble) const;
    bool parseFloat(float &outFloat) const;
    bool parseInt(int64_t &outInt) const;
    bool parseInt(int &outInt) const;
    bool parseUInt(uint32_t &outInt) const;

    bool parseNumber(double& outDouble) const;
    bool parseNumber(float& outFloat) const;

    bool parseBool(bool &outBool) const;
    bool parseBuffer(PodVector<uint8_t> &outBuffer) const;

    bool hasChild(std::string_view childName) const;

    int getChildCount() const { return ( int )children.size(); }
    const JSONBlock &getChild(int index) const;
    const JSONBlock &getChild(std::string_view childName) const;

    bool print() const;

    Vector< JSONBlock > children;
    std::string_view blockName;

    bool named = false;

    bool valueBool = false;
    std::string_view valueStr;
    double valueDbl = -1.0;
    int64_t valueInt = -1;

    int jType = 0;

    static const JSONBlock emptyBlock;
};
