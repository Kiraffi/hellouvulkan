#pragma once

#include "mytypes.h"

#include <container/string.h>
#include <container/stringview.h>

#include <math/quaternion.h>
#include <math/vector3.h>

class WriteJson
{
public:
    static constexpr u32 INDENT_INSPACES = 4u;

    WriteJson(u32 magicNumber, u32 versionNumber);
    bool finishWrite();

    bool addString(StringView name, StringView value, bool quoteValue = true);
    bool addInteger(StringView name, i64 number);
    bool addNumber(StringView name, double number);
    bool addBool(StringView name, bool b);

    // while in array
    bool addNumberArray(StringView name, const double *number, u32 numberCount);
    bool addNumberArray(StringView name, const float *number, u32 numberCount);
    bool addIntegerArray(StringView name, const i64 *number, u32 numberCount);
    bool addIntegerArray(StringView name, const i32 *number, u32 numberCount);

    bool addMagicNumberAndVersion(u32 magicNumber, u32 versionNumber);

    bool addArray(StringView name);
    bool endArray();

    bool addObject(StringView name);
    bool addObject();
    bool addObject(u32 magicNumber, u32 versionNumber);
    bool endObject();

    bool writeVec2(StringView name, const Vector2 &v);
    bool writeVec3(StringView name, const Vector3 &v);
    bool writeVec4(StringView name, const Vector4 &v);
    bool writeQuat(StringView name, const Quaternion &q);

    const String &getString() const { return writtenJson; }
    bool isValid() const { return valid; }
private:
    bool addNamedString(StringView str, bool addQuotes);
    bool addIndentSpaces();
    bool addTypeArray(StringView name, const void *number, u32 bits, u32 numberCount,
        void (*f) (String &writtenJson, void *writePlace));
    String writtenJson;
    String blockTypes;
    u32 indentAmount = 0;
    bool valid = true;
};
