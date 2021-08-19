#include "json.h"


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

	if(index + 4 < endIndex && memcmp(&buffer [index], "true", 4) == 0)
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
	bool isArray = inOutBlock.isArray();

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
	else if(cc = '[')
	{
		res->jType |= JSONBlock::ARRAY_TYPE;
		res->jType |= JSONBlock::VALID_TYPE;
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
	return true;
}

bool JSONBlock::parseDouble(double &outDouble) const
{
	return true;

}

bool JSONBlock::parseInt(int64_t &outInt) const
{
	return true;

}