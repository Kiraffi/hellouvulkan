#include "writejson.h"

#include <ctype.h>
#include <string.h>

WriteJson::WriteJson(uint32_t magicNumber, uint32_t versionNumber) : indentAmount(INDENT_INSPACES), valid(true)
{
    writtenJson.append("{\n");
    addMagicNumberAndVersion(magicNumber, versionNumber);
}

bool WriteJson::addString(StringView name, StringView value, bool addQuotes)
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
    writtenJson.append(" : ");
    addNamedString(value, addQuotes);
    writtenJson.append(", \n");
    return valid;
}

bool WriteJson::addInteger(StringView name, int64_t number)
{
    String s;
    s.append(number);
    return addString(name, s.getStr(), false);
}

bool WriteJson::addNumber(StringView name, double number)
{
    String s;
    s.append(number);
    return addString(name, s.getStr(), false);
}

bool WriteJson::addBool(StringView name, bool b)
{
    return addString(name, b ? "true" : "false", false);
}

bool WriteJson::addMagicNumberAndVersion(uint32_t magicNumber, uint32_t versionNumber)
{
    if(!valid)
        return false;
    addIndentSpaces();
    writtenJson.append("\"magicNumber\" : ");
    writtenJson.append(magicNumber);
    writtenJson.append(",\n");
    addIndentSpaces();
    writtenJson.append("\"versionNumber\" : ");
    writtenJson.append(versionNumber);
    writtenJson.append(",\n");
    return valid;
}

bool WriteJson::addArray(StringView name)
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
    writtenJson.append(" : [\n");

    indentAmount += INDENT_INSPACES;
    blockTypes.append("]");
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
    if(blockTypes[blockTypes.size() - 1] != ']')
    {
        valid = false;
        return false;
    }
    indentAmount -= INDENT_INSPACES;
    addIndentSpaces();
    writtenJson.append("],\n");
    blockTypes.erase(blockTypes.size() - 1);
    return valid;
}

bool WriteJson::addObject(StringView name)
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
        writtenJson.append(" : ");
    }
    writtenJson.append("{\n");

    indentAmount += INDENT_INSPACES;
    blockTypes.append("}");
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
    writtenJson.append("{\n");

    indentAmount += INDENT_INSPACES;
    blockTypes.append("}");
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
printf("%s\nsize: %i\n", blockTypes.getStr(), blockTypes.getSize());
    if(blockTypes.size() == 0 && indentAmount > INDENT_INSPACES)
    {
        valid = false;
        return false;
    }
    if(blockTypes[blockTypes.size() - 1] != '}')
    {
        valid = false;
        return false;
    }
    indentAmount -= INDENT_INSPACES;
    addIndentSpaces();
    writtenJson.append("},\n");
    blockTypes.erase(blockTypes.size() - 1);
    return valid;
}

bool WriteJson::writeVec3(StringView name, const Vector3 &v)
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

    writtenJson.append(" : [ ");
    writtenJson.append(v.x);
    writtenJson.append(", ");
    writtenJson.append(v.y);
    writtenJson.append(", ");
    writtenJson.append(v.z);
    writtenJson.append(" ],\n");
    return true;
}
bool WriteJson::writeQuat(StringView name, const Quaternion &q)
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

    writtenJson.append(" : [ ");
    writtenJson.append(q.v.x);
    writtenJson.append(", ");
    writtenJson.append(q.v.y);
    writtenJson.append(", ");
    writtenJson.append(q.v.z);
    writtenJson.append(", ");
    writtenJson.append(q.w);
    writtenJson.append(" ],\n");
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
    writtenJson.append("}\n\0");

    printf("output:\n%s", writtenJson.getStr());
    return true;
}


bool WriteJson::addNamedString(StringView name, bool quoteValue)
{
    if(name.size() == 0)
        return false;

    if(quoteValue)
        writtenJson.append("\"");

    writtenJson.append(name.ptr, name.length);

    if(quoteValue)
        writtenJson.append("\"");
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

