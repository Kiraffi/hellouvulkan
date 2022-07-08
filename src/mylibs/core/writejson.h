#pragma once

#include "mytypes.h"

#include <container/string.h>
#include <container/stringview.h>

#include <math/quaternion.h>
#include <math/vector3.h>

class WriteJson
{
public:
    static constexpr uint32_t INDENT_INSPACES = 4u;

    WriteJson(uint32_t magicNumber, uint32_t versionNumber);
    bool finishWrite();

    bool addString(StringView name, StringView value, bool quoteValue = true);
    bool addInteger(StringView name, int64_t number);
    bool addNumber(StringView name, double number);
    bool addBool(StringView name, bool b);

    bool addMagicNumberAndVersion(uint32_t magicNumber, uint32_t versionNumber);

    bool addArray(StringView name);
    bool endArray();

    bool addObject(StringView name);
    bool addObject();
    bool addObject(uint32_t magicNumber, uint32_t versionNumber);
    bool endObject();

    bool writeVec3(StringView name, const Vector3 &v);
    bool writeQuat(StringView name, const Quaternion &q);

    const String &getString() const { return writtenJson; }
    bool isValid() const { return valid; }
private:
    bool addNamedString(StringView str, bool addQuotes);
    bool addIndentSpaces();
    String writtenJson;
    String blockTypes;
    uint32_t indentAmount = 0;
    bool valid = true;
};
