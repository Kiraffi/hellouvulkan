#include "writejson.h"

#include <ctype.h>
#include <string.h>
#include <string>

WriteJson::WriteJson(uint32_t magicNumber, uint32_t versionNumber) : indentAmount(4), isValid(true)
{
    writtenJson += "{\n    \"magicNumber\" : ";
    writtenJson += std::to_string(magicNumber);
    writtenJson += ",\n    \"versionNumber\" : ";
    writtenJson += std::to_string(versionNumber);
    writtenJson += ",\n";
}

bool WriteJson::addString(std::string_view name, std::string_view value, bool addQuotes)
{
    if(!isValid)
        return false;

    if(name.size() == 0 || value.size() == 0)
    {
        isValid = false;
        return false;
    }
    addIndentSpaces();
    addNamedString(name, true);
    writtenJson += " : ";
    addNamedString(value, addQuotes);
    writtenJson += ", \n";
    return isValid;
}

bool WriteJson::addInteger(std::string_view name, int64_t number) { return addString(name, std::to_string(number), false); }
bool WriteJson::addNumber(std::string_view name, double number) { return addString(name, std::to_string(number), false); }
bool WriteJson::addBool(std::string_view name, bool b) { return addString(name, b ? "true" : "false", false); }


bool WriteJson::addArray(std::string_view name)
{
    if(!isValid)
        return false;

    if(name.size() == 0)
    {
        isValid = false;
        return false;
    }
    addIndentSpaces();
    addNamedString(name, true);
    writtenJson += " : [\n";

    indentAmount += 4u;
    blockTypes += "]";
    return isValid;
}


bool WriteJson::endArray()
{
    if(!isValid)
        return false;

    if(blockTypes.size() == 0 && indentAmount > 4)
    {
        isValid = false;
        return false;
    }
    if(blockTypes.back() != ']')
    {
        isValid = false;
        return false;
    }
    indentAmount -= 4u;
    addIndentSpaces();
    writtenJson += "],\n";
    blockTypes.erase(blockTypes.size() - 1);
    return isValid;
}

bool WriteJson::addObject(std::string_view name)
{
    if(!isValid)
        return false;

    if(blockTypes.size() < 1 || blockTypes[blockTypes.size() - 1] != ']')
    {
        isValid = false;
        return false;
    }
    addIndentSpaces();
    if(name.size() > 0)
    {
        addNamedString(name, true);
        writtenJson += " : ";
    }
    writtenJson += "{\n";

    indentAmount += 4u;
    blockTypes += "}";
    return isValid;
}


bool WriteJson::endObject()
{
    if(!isValid)
        return false;

    if(blockTypes.size() == 0 && indentAmount > 4)
    {
        isValid = false;
        return false;
    }
    if(blockTypes.back() != '}')
    {
        isValid = false;
        return false;
    }
    indentAmount -= 4u;
    addIndentSpaces();
    writtenJson += "},\n";
    blockTypes.erase(blockTypes.size() - 1);
    return isValid;
}

bool WriteJson::writeVec3(std::string_view name, const Vector3 &v)
{
    if(!isValid)
        return false;

    if(name.size() == 0)
    {
        isValid = false;
        return false;
    }
    addIndentSpaces();
    addNamedString(name, true);
    
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
    if(!isValid)
        return false;

    if(name.size() == 0)
    {
        isValid = false;
        return false;
    }
    addIndentSpaces();
    addNamedString(name, true);

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



bool WriteJson::finishWrite()
{
    if(!isValid)
        return false;

    if(indentAmount != 4)
    {
        isValid = false;
        return false;
    }
    if(blockTypes.size() != 0)
    {
        isValid = false;
        return false;
    }
    writtenJson += "}\n";

    printf("output:\n%s", writtenJson.c_str());
    return true;
}


bool WriteJson::addNamedString(std::string_view name, bool quoteValue)
{
    if(name.size() == 0)
        return false;

    if(quoteValue)
        writtenJson += "\"";

    writtenJson += std::string(name);

    if(quoteValue)
        writtenJson += "\"";
    return true;
}

bool WriteJson::addIndentSpaces()
{
    if(!isValid)
        return false;

    if(indentAmount == 0 || indentAmount > 128)
    {
        isValid = false;
        return false;
    }

    writtenJson.append(indentAmount, ' ');
    return true;
}

