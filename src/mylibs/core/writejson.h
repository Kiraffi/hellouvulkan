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
    WriteJson(uint32_t magicNumber, uint32_t versionNumber);
    bool finishWrite(std::string_view filename);

    bool addString(std::string_view name, std::string_view value, bool quoteValue = true);
    bool addInteger(std::string_view name, int64_t number);
    bool addNumber(std::string_view name, double number);
    bool addBool(std::string_view name, bool b);

    bool addArray(std::string_view name);
    bool endArray();

    bool addObject(std::string_view name);
    bool endObject();

    bool writeVec3(std::string_view name, const Vector3 &v);
    bool writeQuat(std::string_view name, const Quaternion &q);

private:
    std::string writtenJson;
    std::string blockTypes;
    uint32_t indentAmount = 0;

};
