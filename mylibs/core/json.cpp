#include "json.h"
#include <memory.h>

const JSONBlock JSONBlock::emptyBlock = { };


struct JSONMarker
{
    JSONMarker(int start) { startIndex = start; }
    JSONMarker(int start, int end): startIndex(start), endIndex(end) { }

    int startIndex = 0;
    int endIndex = 0;
    int currentIndex = 0;
};

static bool parseObject(const std::vector<char> &buffer, JSONMarker &marker, JSONBlock &inOutBlock);
static bool parseValue(const std::vector<char> &buffer, JSONMarker &marker, JSONBlock &inOutBlock);

static bool printBlock(const JSONBlock &bl, int spaces = 0)
{

    for(int i = 0; i < spaces; ++i)
        printf("  ");

    if(!bl.blockName.empty())
        printf("%s: ", bl.blockName.c_str());

    if(!bl.isValid())
    {
        printf("INVALID!\n");
        return false;
    }

    if(bl.isObject())
    {

        printf("Object\n");
        for(const JSONBlock &child : bl.children)
        {
            if(!printBlock(child, spaces + 1))
                return false;
        }
    }
    else if(bl.isArray())
    {
        printf("Array\n");
        for(const JSONBlock &child : bl.children)
        {
            printBlock(child, spaces + 1);
        }
    }
    else if(bl.isBool())
    {
        bool v = false;
        if(!bl.parseBool(v))
        {
            printf("FAILED PARSE BOOL\n");
            return false;
        }
        printf("Bool %i\n", v);
    }
    else if(bl.isInt())
    {
        int64_t v = -2312;
        if(!bl.parseInt(v))
        {
            printf("FAILED PARSE INT\n");
            return false;
        }
        printf("Int %i\n", ( int )v);
    }
    else if(bl.isDouble())
    {
        double v = -2312.0;
        if(!bl.parseDouble(v))
        {
            printf("FAILED PARSE DOUBLE\n");
            return false;
        }
        printf("Double %f\n", ( float )v);
    }
    else if(bl.isString())
    {
        std::string s;
        if(!bl.parseString(s))
        {
            printf("FAILED PARSE STRING\n");
            return false;
        }
        printf("Str %s\n", s.c_str());
    }
    else
    {
        printf("IS NOT KNOWN TYPE\n");
        return false;

    }
    return true;
}


static bool skipWord(const std::vector<char> &buffer, JSONMarker &marker)
{
    int &index = marker.currentIndex;
    int sz = marker.endIndex;
    while(index < sz && !isspace(buffer [index]))
        ++index;

    return index < sz;
}


static bool skipWhiteSpace(const std::vector<char> &buffer, JSONMarker &marker)
{
    int &index = marker.currentIndex;
    int sz = marker.endIndex;
    while(index < sz && isspace(buffer [index]))
        ++index;

    return index < sz;
}
bool parseBetweenMarkers(const std::vector<char> &buffer, JSONMarker &marker, char beginChar, char endChar, bool ignoreBackSlashedChar = true)
{
    int &index = marker.currentIndex;
    int endIndex = marker.endIndex;

    if(buffer [index] != beginChar)
        return false;
    ++index;

    int openCount = 1;
    bool backSlash = false;
    while(index < endIndex)
    {
        char c = buffer [index];
        if(!ignoreBackSlashedChar && c == '\\')
        {
            backSlash = !backSlash;
        }
        // in case end char and startchar are same, consider always checking end char first
        else if(c == endChar && ( ignoreBackSlashedChar || !backSlash ))
        {
            --openCount;
            if(openCount == 0)
            {
                return true;
            }
        }
        else if(c == beginChar && ( ignoreBackSlashedChar || !backSlash ))
        {
            ++openCount;
        }
        if(!ignoreBackSlashedChar && c != '\\')
        {
            backSlash = false;
        }
        ++index;
    }
    return false;
}

static bool parseString(const std::vector<char> &buffer, JSONMarker &marker, std::string &outStr)
{
    int &index = marker.currentIndex;

    int startIndex = index + 1;
    if(!parseBetweenMarkers(buffer, marker, '"', '"', false))
        return false;

    int l = marker.currentIndex - startIndex;

    outStr.resize(l);
    for(int i = 0; i < l; ++i)
    {
        outStr [i] = buffer [startIndex + i];
    }

    if(l < 0 || index >= marker.endIndex || buffer [index] != '"')
        return false;
    ++index;
    if(index + 1 < marker.endIndex && buffer[index] == ',')
        ++index;
    return true;
}

static int parseInt(const std::vector<char> &buffer, JSONMarker &marker, int64_t &outResult)
{
    int numCount = 0;
    int &index = marker.currentIndex;
    int sz = marker.endIndex;
    if(!skipWhiteSpace(buffer, marker))
        return -1;

    bool neg = false;
    if(buffer [index] == '-')
    {
        neg = true;
        index++;
    }
    outResult = 0;
    while(index < sz)
    {
        char c = buffer [index];
        if(c >= '0' && c <= '9')
        {
            ++numCount;
            if(numCount > 18)
                return -1;
            outResult = outResult * 10 + int(c - '0');
        }
        else
            break;

        ++index;
    }
    if(neg)
        outResult = -outResult;
    if(index < sz && buffer [index] == ',')
        ++index;

    return numCount;
}


bool isNumOrMinus(char c)
{
    return( c == '-' || ( c >= '0' && c <= '9' ) );
}

bool isNum(char c)
{
    return(( c >= '0' && c <= '9' ));
}

bool getNumber(const std::vector<char> &buffer, JSONMarker &marker, JSONBlock &inOutBlock)
{
    int &index = marker.currentIndex;
    int endIndex = marker.endIndex;
    if(index < endIndex && !isNumOrMinus(buffer [index]))
        return false;
    bool neg = false;
    if(buffer [index] == '-')
    {
        neg = true;
        ++index;
    }
    int64_t v = -1;
    int numCount = parseInt(buffer, marker, v);
    if(numCount <= 0 || index > marker.endIndex)
        return false;

    inOutBlock.valueInt = neg ? -v : v;
    inOutBlock.valueDbl = neg ? -v : v;

    if(buffer [index] == '.')
    {
        ++index;
        double d = v;

        numCount = parseInt(buffer, marker, v);
        if(numCount <= 0 || index > marker.endIndex || v < 0)
            return false;

        double div = 1.0;
        for(int i = 0; i < numCount; ++i)
            div *= 10.0;

        d += ( double )v / div;

        if(buffer [index] == 'e')
        {
            ++index;
            numCount = parseInt(buffer, marker, v);
            if(numCount <= 0 || index > marker.endIndex)
                return false;
            div = 1.0;
            if(v > 0)
            {
                for(int i = 0; i < v; ++i)
                    div *= 10.0;
                d *= div;
            }
            else
            {
                for(int i = 0; i > v; --i)
                    div *= 10.0;
                d /= div;
            }

        }

        inOutBlock.jType |= JSONBlock::DOUBLE_TYPE | JSONBlock::VALID_TYPE;
        inOutBlock.valueDbl = neg ? -d : d;
    }
    else
    {
        inOutBlock.jType |= JSONBlock::INT_TYPE | JSONBlock::VALID_TYPE;
        inOutBlock.valueInt = neg ? -v : v;
    }
    return true;
}

bool tryParseBoolean(const std::vector<char> &buffer, JSONMarker &marker, JSONBlock &inOutBlock)
{
    int &index = marker.currentIndex;
    int endIndex = marker.endIndex;

    if(index + 4 < endIndex && memcmp((const void *)(&(buffer [index])), "true", 4) == 0)
    {
        inOutBlock.valueBool = true;
        inOutBlock.jType |= JSONBlock::BOOL_TYPE | JSONBlock::VALID_TYPE;
        index += 4;
    }
    else if(index + 5 < endIndex && memcmp(&buffer [index], "false", 5) == 0)
    {
        inOutBlock.valueBool = false;
        inOutBlock.jType |= JSONBlock::BOOL_TYPE | JSONBlock::VALID_TYPE;
        index += 5;
    }
    else
    {
        return false;
    }
    if(index + 1 < marker.endIndex && buffer [index] == ',')
        ++index;

    return true;
}

static bool parseValue(const std::vector<char> &buffer, JSONMarker &marker, JSONBlock &inOutBlock)
{
    int &index = marker.currentIndex;
    int endIndex = marker.endIndex;
    bool isArray = (inOutBlock.jType & JSONBlock::ARRAY_TYPE) == JSONBlock::ARRAY_TYPE;

    JSONBlock *res = &inOutBlock;
    if(isArray)
    {
        inOutBlock.children.emplace_back(JSONBlock());
        res = &inOutBlock.children.back();
    }

    if(!skipWhiteSpace(buffer, marker))
        return false;

    char cc = buffer [index];
    if(isNumOrMinus(cc))
    {
        if(!getNumber(buffer, marker, *res))
            return false;
    }
    else if(tryParseBoolean(buffer, marker, *res))
    {

    }
    else if(cc == '"')
    {
        if(!parseString(buffer, marker, res->valueStr))
            return false;
        res->jType |= JSONBlock::STRING_TYPE;
    }
    else if(cc == '{')
    {
        JSONMarker childMarker(index, -1);
        childMarker.currentIndex = index;
        if(!parseBetweenMarkers(buffer, marker, '{', '}', false))
            return false;
        childMarker.endIndex = index;
        if(!parseObject(buffer, childMarker, *res))
            return false;
        ++index;
    }
    else if(cc == '[')
    {
        res->jType |= JSONBlock::ARRAY_TYPE;
        JSONMarker childMarker(index, -1);
        childMarker.currentIndex = index + 1;
        if(!parseBetweenMarkers(buffer, marker, '[', ']', false))
            return false;
        childMarker.endIndex = index;

        while(childMarker.currentIndex < childMarker.endIndex)
        {
            if(!parseValue(buffer, childMarker, *res))
                return false;

            skipWhiteSpace(buffer, childMarker);
            if(childMarker.currentIndex < childMarker.endIndex && buffer [childMarker.currentIndex] == ',')
                ++childMarker.currentIndex;
        }
        ++index;
    }
    else
    {
        return false;
    }

    skipWhiteSpace(buffer, marker);
    if(index < buffer.size() && buffer [index] == ',')
        ++index;

    res->jType |= JSONBlock::VALID_TYPE;
    return true;

}


static bool parseObject(const std::vector<char> &buffer, JSONMarker &marker, JSONBlock &inOutBlock)
{
    int &index = marker.currentIndex;
    int endIndex = marker.endIndex;
    if(buffer [index] != '{' || index >= endIndex)
        return false;
    ++index;
    inOutBlock.jType |= JSONBlock::OBJECT_TYPE;
    if(!skipWhiteSpace(buffer, marker))
        return false;
    while(index < endIndex)
    {
        inOutBlock.children.emplace_back(JSONBlock());
        JSONBlock &childBlock = inOutBlock.children.back();

        if(!parseString(buffer, marker, childBlock.blockName))
            return false;

        if(!skipWhiteSpace(buffer, marker))
            return false;

        if(index >= endIndex || buffer [index] != ':')
            return false;

        ++index;
        if(!skipWhiteSpace(buffer, marker))
            return false;

        if(!parseValue(buffer, marker, childBlock))
            return false;

        skipWhiteSpace(buffer, marker);
        if(index < endIndex && buffer [index] == ',')
            ++index;

        skipWhiteSpace(buffer, marker);
    }
    if(index >= buffer.size() || index != endIndex || buffer [index] != '}')
        return false;
    ++index;
    inOutBlock.jType |= JSONBlock::VALID_TYPE;
    return true;
}



bool JSONBlock::parseJSON(const std::vector<char> &data)
{
    if(data.size() <= 2)
        return false;

    JSONMarker marker(0, ( int )data.size() - 1);
    skipWhiteSpace(data, marker);
    while(marker.endIndex > 0 && data [marker.endIndex] != '}')
        --marker.endIndex;
    if(marker.endIndex == 0)
        return false;

    return parseObject(data, marker, *this);
}

bool JSONBlock::parseString(std::string &outString) const
{
    if(!isString())
        return false;

    outString = valueStr;
    return true;
}

bool JSONBlock::parseDouble(double &outDouble) const
{
    if(!isDouble())
        return false;

    outDouble = valueDbl;
    return true;

}

bool JSONBlock::parseFloat(float &outFloat) const
{
    double d = 0.0;
    if(!parseDouble(d))
        return false;

    outFloat = float(d);
    return true;

}

bool JSONBlock::parseInt(int64_t &outInt) const
{
    if(!isInt())
        return false;

    outInt = valueInt;
    return true;

}

bool JSONBlock::parseInt(int &outInt) const
{
    int64_t v = 0;
    if(!parseInt(v))
        return false;
    if(v < -0x1'0000'0000LL || v > 0xffff'ffffLL)
        return false;

    outInt = int(v);
    return true;

}

bool JSONBlock::parseUInt(uint32_t &outInt) const
{
    int64_t v = 0;
    if(!parseInt(v))
        return false;
    if(v < 0 || v > 0xffff'ffffLL)
        return false;

    outInt = uint32_t(v);
    return true;

}


bool JSONBlock::parseNumber(float& outFloat) const
{
    if(!isInt() && !isDouble())
        return false;

    outFloat = (float)valueDbl;
        return true;
}


bool JSONBlock::parseBool(bool &outBool) const
{
    if(!isBool())
        return false;

    outBool = valueBool;
    return true;

}


bool JSONBlock::parseBuffer(std::vector<uint8_t> &outBuffer) const
{
    if(!isString())
        return false;

    int strLen = valueStr.length();

    if(strLen < 37)
        return false;

    if(memcmp(valueStr.c_str(), "data:application/octet-stream;base64,", 37) != 0)
        return false;

    outBuffer.reserve(strLen - 37);
    outBuffer.clear();

    int byteOffset = 0;
    uint8_t currentByte = 0u;

    for(int charIndex = 37; charIndex < strLen; ++charIndex)
    {
        char c = valueStr [charIndex];
        uint8_t readByte = 0u;

        if(c >= 'A' && c <= 'Z') readByte = ( c - 'A' );
        if(c >= 'a' && c <= 'z') readByte = ( c - 'a' ) + 26;
        if(c >= '0' && c <= '9') readByte = ( c - '0' ) + 52;
        if(c == '+') readByte = 62;
        if(c == '/') readByte = 63;
        if(c == '=') return true;
        if(c == '\"') break;

        if(byteOffset == 0)
        {
            currentByte = readByte << 2;
        }
        else if(byteOffset == 1)
        {
            currentByte |= ( readByte >> 4 );
            outBuffer.push_back(currentByte);
            currentByte = ( readByte & 15 ) << 4;
        }
        else if(byteOffset == 2)
        {
            currentByte |= ( readByte >> 2 );
            outBuffer.push_back(currentByte);
            currentByte = ( readByte & 3 ) << 6;
        }
        else if(byteOffset == 3)
        {
            currentByte |= ( readByte );
            outBuffer.push_back(currentByte);
            currentByte = 0;
        }
        byteOffset = ( byteOffset + 1 ) % 4;
    }
    if(currentByte != 0)
        outBuffer.push_back(currentByte);


    return true;
}

bool JSONBlock::hasChild(std::string_view childName) const
{
    for(const JSONBlock &child : children)
    {
        if(child.blockName == childName)
            return true;
    }

    return false;

}

const JSONBlock &JSONBlock::getChild(int index) const
{
    if(index < 0 || index >= children.size())
        return emptyBlock;

    return children [index];
}

const JSONBlock &JSONBlock::getChild(std::string_view childName) const
{
    for(const JSONBlock &child : children)
    {
        if(child.blockName == childName)
            return child;
    }

    return emptyBlock;
}

bool JSONBlock::print() const
{
    return printBlock((*this), 0);
}

