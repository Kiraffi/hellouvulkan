#pragma once

#include "mytypes.h"

#include <container/arraysliceview.h>
#include <container/podvector.h>
#include <container/vector.h>
#include <core/json.h>

#include <math/quaternion.h>
#include <math/vector3.h>

#include <string_view>

class WriteJson
{
public:
    static constexpr uint32_t INDENT_INSPACES = 4u;

    WriteJson(uint32_t magicNumber, uint32_t versionNumber);
    bool finishWrite();

    bool addString(std::string_view name, std::string_view value, bool quoteValue = true);
    bool addInteger(std::string_view name, int64_t number);
    bool addNumber(std::string_view name, double number);
    bool addBool(std::string_view name, bool b);

    bool addMagicNumberAndVersion(uint32_t magicNumber, uint32_t versionNumber);

    bool addArray(std::string_view name);
    bool endArray();

    bool addObject(std::string_view name);
    bool addObject();
    bool addObject(uint32_t magicNumber, uint32_t versionNumber);
    bool endObject();

    bool writeVec3(std::string_view name, const Vector3 &v);
    bool writeQuat(std::string_view name, const Quaternion &q);

    const std::string &getString() const { return writtenJson; }
    bool isValid() const { return valid; }
private:
    bool addNamedString(std::string_view str, bool addQuotes);
    bool addIndentSpaces();
    std::string writtenJson;
    std::string blockTypes;
    uint32_t indentAmount = 0;
    bool valid = true;
};
