#include "json.h"


struct JSONMarker
{
	JSONMarker(int start) { startIndex = start; }
	JSONMarker(int start, int end): startIndex(start), endIndex(end) { }

	int startIndex = 0;
	int endIndex = 0;
	int currentIndex = 0;
};

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

static bool parseString(const std::vector<char> &buffer, JSONMarker &marker, std::string &outStr)
{
	int &index = marker.currentIndex;
	if(index >= buffer.size() || buffer [index] != '"' || marker.endIndex > buffer.size() )
		return false;
	++index;
	int startIndex = marker.currentIndex;
	int endInd = -1;
	bool prevBlackSlash = false;
	for(; index < marker.endIndex; ++index)
	{
		char c = buffer [index];
		if(c == '\\')
			prevBlackSlash = !prevBlackSlash;
		else
		{
			if(c == '"' && !prevBlackSlash)
			{
				endInd = index;
				break;
			}
			prevBlackSlash = false;
		}
	}
	int l = endInd - startIndex;

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

	return numCount > 0;
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
		inOutBlock.valueDbl = d;
	}
	else
	{
		inOutBlock.jType |= JSONBlock::INT_TYPE | JSONBlock::VALID_TYPE;
		inOutBlock.valueInt = v;
	}
	return true;
}

bool tryParseBoolean(const std::vector<char> &buffer, JSONMarker &marker, JSONBlock &inOutBlock)
{
	int &index = marker.currentIndex;
	int endIndex = marker.endIndex;
	bool isArray = inOutBlock.isArray();
	if(index + 4 < endIndex && isArray && memcmp(&buffer [index], "true", 4) == 0)
	{
		inOutBlock.children.emplace_back(JSONBlock());
		JSONBlock &child = inOutBlock.children.back();
		child.valueBool = true;

		child.jType |= JSONBlock::BOOL_TYPE | JSONBlock::VALID_TYPE;
		index += 4;
	}
	else if(index + 5 < endIndex && isArray && memcmp(&buffer [index], "false", 5) == 0 && isArray)
	{
		inOutBlock.children.emplace_back(JSONBlock());
		JSONBlock &child = inOutBlock.children.back();
		child.valueBool = false;

		child.jType |= JSONBlock::BOOL_TYPE | JSONBlock::VALID_TYPE;
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

bool getJSONContent(const std::vector<char> &buffer, JSONMarker &marker, JSONBlock &inOutBlock)
{
	int &index = marker.currentIndex;
	int endIndex = marker.endIndex;
	bool isArray = inOutBlock.isArray();
	while(index < endIndex)
	{
		if(!skipWhiteSpace(buffer, marker))
			return ( index == marker.endIndex );
		
		char cc = buffer [index];
		if(isNumOrMinus(cc) && isArray)
		{
			inOutBlock.children.emplace_back(JSONBlock());
			JSONBlock &child = inOutBlock.children.back();
			if(!getNumber(buffer, marker, child))
				return false;
		}
		else if(tryParseBoolean(buffer, marker, inOutBlock))
		{

		}
		else if(cc == '"')
		{
			inOutBlock.children.emplace_back(JSONBlock());
			JSONBlock &child = inOutBlock.children.back();
			child.named = true;

			if(!parseString(buffer, marker, child.blockName))
				return false;

			if(!skipWhiteSpace(buffer, marker))
				return false;

			if(index >= endIndex || buffer [index] != ':')
			{
				if(!isArray)
					return false;
			}
			else
			{

				++index;

				if(!skipWhiteSpace(buffer, marker))
					return false;

				if(index >= endIndex)
					return false;

				char c = buffer [index];
				if(c == '[' || c == '{')
				{
					if(!getJSONContent(buffer, marker, child))
						return false;
				}

				else if(isNumOrMinus(c))
				{
					if(!getNumber(buffer, marker, child))
						return false;

				}
				else if(tryParseBoolean(buffer, marker, inOutBlock))
				{

				}

				else if(c == '"')
				{
					if(!parseString(buffer, marker, child.valueStr))
						return false;

					child.jType |= JSONBlock::STRING_TYPE | JSONBlock::VALID_TYPE;

				}
				else
				{
					return false;
				}
			}
		}
		else
		{
			char findOpen = '[';
			char findClosed = ']';

			if(buffer [index] == '[')
			{
				findOpen = '[';
				findClosed = ']';
				inOutBlock.jType |= JSONBlock::ARRAY_TYPE;
			}
			else if(buffer [index] == '{')
			{
				findOpen = '{';
				findClosed = '}';
				inOutBlock.jType |= JSONBlock::OBJECT_TYPE;
			}
			else
			{
				return false;
			}

			int bracketCount = 1;

			JSONMarker childMarker(index, -1);

			++index;
			childMarker.currentIndex = index;
			while(index < endIndex)
			{
				char c = buffer [index];
				if(c == findOpen)
				{
					++bracketCount;
				}
				else if(c == findClosed)
				{
					--bracketCount;
					if(bracketCount == 0)
					{
						childMarker.endIndex = index;
						++index;
						break;
					}
				}
				++index;
			}

			if(childMarker.endIndex == -1)
				return false;
			skipWhiteSpace(buffer, marker);
			if(index < endIndex && buffer [index] == ',')
				++index;

			if(inOutBlock.named && !isArray)
			{
				if(!getJSONContent(buffer, childMarker, inOutBlock))
					return false;
			}
			else
			{
				inOutBlock.children.emplace_back(JSONBlock());
				JSONBlock &child = inOutBlock.children.back();
				if(!getJSONContent(buffer, childMarker, inOutBlock))
					return false;

			}

		}
		skipWhiteSpace(buffer, marker);
		if(index < buffer.size() && buffer [index] == ',')
			++index;
	}
	return true;
}


bool JSONBlock::parseJSON(const std::vector<char> &data)
{
	if(data.size() <= 2)
		return false;

	JSONMarker marker(0, ( int )data.size());
	named = true;
	return getJSONContent(data, marker, (*this));
}

bool JSONBlock::parseString(std::string &outString)
{
	return true;
}

bool JSONBlock::parseDouble(double &outDouble)
{
	return true;

}

bool JSONBlock::parseInt(int64_t &outInt)
{
	return true;

}