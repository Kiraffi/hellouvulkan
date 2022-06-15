#include "writejson.h"

#include <ctype.h>
#include <string.h>
#include <string>

WriteJson::WriteJson(uint32_t magicNumber, uint32_t versionNumber) : indentAmount(INDENT_INSPACES), valid(true)
{
    writtenJson += "{\n";
    addMagicNumberAndVersion(magicNumber, versionNumber);
}

bool WriteJson::addString(std::string_view name, std::string_view value, bool addQuotes)
{
    if(!valid)
        return false;

    if(name.size() == 0 || value.size() == 0)
    {
        valid = false;
        return false;
    }
    addIndentSpaces();
    addNamedString(name, true);
    writtenJson += " : ";
    addNamedString(value, addQuotes);
    writtenJson += ", \n";
    return valid;
}

bool WriteJson::addInteger(std::string_view name, int64_t number) { return addString(name, std::to_string(number), false); }
bool WriteJson::addNumber(std::string_view name, double number) { return addString(name, std::to_string(number), false); }
bool WriteJson::addBool(std::string_view name, bool b) { return addString(name, b ? "true" : "false", false); }

bool WriteJson::addMagicNumberAndVersion(uint32_t magicNumber, uint32_t versionNumber)
{
    if(!valid)
        return false;
    addIndentSpaces();
    writtenJson += "\"magicNumber\" : ";
    writtenJson += std::to_string(magicNumber);
    writtenJson += ",\n";
    addIndentSpaces();
    writtenJson += "\"versionNumber\" : ";
    writtenJson += std::to_string(versionNumber);
    writtenJson += ",\n";
    return valid;
}

bool WriteJson::addArray(std::string_view name)
{
    if(!valid)
        return false;

    if(name.size() == 0)
    {
        valid = false;
        return false;
    }
    addIndentSpaces();
    addNamedString(name, true);
    writtenJson += " : [\n";

    indentAmount += INDENT_INSPACES;
    blockTypes += "]";
    return valid;
}


bool WriteJson::endArray()
{
    if(!valid)
        return false;

    if(blockTypes.size() == 0 && indentAmount > INDENT_INSPACES)
    {
        valid = false;
        return false;
    }
    if(blockTypes.back() != ']')
    {
        valid = false;
        return false;
    }
    indentAmount -= INDENT_INSPACES;
    addIndentSpaces();
    writtenJson += "],\n";
    blockTypes.erase(blockTypes.size() - 1);
    return valid;
}

bool WriteJson::addObject(std::string_view name)
{
    if(!valid)
        return false;

    if(name.size() == 0)
    {
        valid = false;
        return false;
    }
    // should not have named objects inside array? Maybe this is actually possible for specs
    if(blockTypes.size() >= 1 && blockTypes[blockTypes.size() - 1] == ']')
    {
        valid = false;
        return false;
    }

    addIndentSpaces();
    if(name.size() > 0)
    {
        addNamedString(name, true);
        writtenJson += " : ";
    }
    writtenJson += "{\n";

    indentAmount += INDENT_INSPACES;
    blockTypes += "}";
    return valid;
}

bool WriteJson::addObject()
{
    if(!valid)
        return false;

    if(blockTypes.size() < 1 || blockTypes[blockTypes.size() - 1] != ']')
    {
        valid = false;
        return false;
    }
    addIndentSpaces();
    writtenJson += "{\n";

    indentAmount += INDENT_INSPACES;
    blockTypes += "}";
    return valid;
}

bool WriteJson::addObject(uint32_t magicNumber, uint32_t versionNumber)
{
    addObject();
    addMagicNumberAndVersion(magicNumber, versionNumber);

    return valid;
}

bool WriteJson::endObject()
{
    if(!valid)
        return false;

    if(blockTypes.size() == 0 && indentAmount > INDENT_INSPACES)
    {
        valid = false;
        return false;
    }
    if(blockTypes.back() != '}')
    {
        valid = false;
        return false;
    }
    indentAmount -= INDENT_INSPACES;
    addIndentSpaces();
    writtenJson += "},\n";
    blockTypes.erase(blockTypes.size() - 1);
    return valid;
}

bool WriteJson::writeVec3(std::string_view name, const Vector3 &v)
{
    if(!valid)
        return false;

    if(name.size() == 0)
    {
        valid = false;
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
    if(!valid)
        return false;

    if(name.size() == 0)
    {
        valid = false;
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
    if(!valid)
        return false;

    if(indentAmount != INDENT_INSPACES)
    {
        valid = false;
        return false;
    }
    if(blockTypes.size() != 0)
    {
        valid = false;
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
    if(!valid)
        return false;

    if(indentAmount == 0 || indentAmount > 128)
    {
        valid = false;
        return false;
    }

    writtenJson.append(indentAmount, ' ');
    return true;
}

