#include "writejson.h"

#include <ctype.h>
#include <string.h>
#include <string>

static bool addNamedString(std::string_view name, bool quoteValue, std::string &inOutString)
{
    if(name.size() == 0)
        return false;
    if(quoteValue)
        inOutString += "\"";

    inOutString += name;

    if(quoteValue)
        inOutString += "\"";
    return true;
}

WriteJson::WriteJson(uint32_t magicNumber, uint32_t versionNumber) : indentAmount(4)
{
    writtenJson += "{\n    \"magicNumber\" : ";
    writtenJson += std::to_string(magicNumber);
    writtenJson += ",\n    \"versionNumber\" : ";
    writtenJson += std::to_string(versionNumber);
    writtenJson += ",\n";
}

bool WriteJson::addString(std::string_view name, std::string_view value, bool quoteValue)
{
    if(name.size() == 0 || value.size() == 0)
        return false;
    for(uint32_t i = 0; i < indentAmount; ++i)
        writtenJson += " ";

    addNamedString(name, true, writtenJson);
    writtenJson += " : ";
    addNamedString(value, quoteValue, writtenJson);
    writtenJson += ", \n";
    return true;
}

bool WriteJson::addInteger(std::string_view name, int64_t number) { return addString(name, std::to_string(number), false); }
bool WriteJson::addNumber(std::string_view name, double number) { return addString(name, std::to_string(number), false); }
bool WriteJson::addBool(std::string_view name, bool b) { return addString(name, b ? "true" : "false", false); }


bool WriteJson::addArray(std::string_view name)
{
    if(name.size() == 0)
        return false;
    for(uint32_t i = 0; i < indentAmount; ++i)
        writtenJson += " ";

    addNamedString(name, true, writtenJson);
    writtenJson += " : [\n";

    indentAmount += 4u;
    blockTypes += "]";
    return true;
}


bool WriteJson::endArray()
{
    if(blockTypes.size() == 0 && indentAmount > 4)
        return false;
    if(blockTypes.back() != ']')
        return false;

    indentAmount -= 4u;
    for(uint32_t i = 0; i < indentAmount; ++i)
        writtenJson += " ";
    writtenJson += "],\n";
    blockTypes.erase(blockTypes.size() - 1);
    return true;
}

bool WriteJson::addObject(std::string_view name)
{
    if(blockTypes.size() < 1 || blockTypes[blockTypes.size() - 1] != ']')
        return false;

    for(uint32_t i = 0; i < indentAmount; ++i)
        writtenJson += " ";
    if(name.size() > 0)
    {
        addNamedString(name, true, writtenJson);
        writtenJson += " : ";
    }
    writtenJson += "{\n";

    indentAmount += 4u;
    blockTypes += "}";
    return true;
}


bool WriteJson::endObject()
{
    if(blockTypes.size() == 0 && indentAmount > 4)
        return false;
    if(blockTypes.back() != '}')
        return false;

    indentAmount -= 4u;
    for(uint32_t i = 0; i < indentAmount; ++i)
        writtenJson += " ";
    writtenJson += "},\n";
    blockTypes.erase(blockTypes.size() - 1);
    return true;
}

bool WriteJson::writeVec3(std::string_view name, const Vector3 &v)
{
    if(name.size() == 0)
        return false;

    for(uint32_t i = 0; i < indentAmount; ++i)
        writtenJson += " ";
    
    addNamedString(name, true, writtenJson);
    
    writtenJson += " : [ ";
    writtenJson += std::to_string(v.x);
    writtenJson += ", ";
    writtenJson += std::to_string(v.y);
    writtenJson += ", ";
    writtenJson += std::to_string(v.z);
    writtenJson += " ],\n";
    return true;
}
bool WriteJson::writeQuat(std::string_view name, const Quaternion &q)
{
    if(name.size() == 0)
        return false;

    for(uint32_t i = 0; i < indentAmount; ++i)
        writtenJson += " ";
    
    addNamedString(name, true, writtenJson);

    writtenJson += " : [ ";
    writtenJson += std::to_string(q.v.x);
    writtenJson += ", ";
    writtenJson += std::to_string(q.v.y);
    writtenJson += ", ";
    writtenJson += std::to_string(q.v.z);
    writtenJson += ", ";
    writtenJson += std::to_string(q.w);
    writtenJson += " ],\n";
    return true;
}



bool WriteJson::finishWrite(std::string_view filename)
{
    if(indentAmount != 4)
        return false;
    if(blockTypes.size() != 0)
        return false;
    writtenJson += "}\n";

    printf("output:\n%s", writtenJson.c_str());
    return true;
}