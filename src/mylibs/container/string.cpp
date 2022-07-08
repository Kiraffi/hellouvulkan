#include "string.h"

#include <core/assert.h>
#include <core/mytypes.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>


static void stringPushbackChar(ByteBuffer &buffer, char c)
{
    buffer.insertIndex(buffer.getSize(), (uint8_t *)(&c));
}

String::String() : buffer(1)
{
    buffer.clear();
    stringPushbackChar(buffer, '\0');
}

String::String(const char *s) : buffer(1)
{
    while (s && *s)
    {
        stringPushbackChar(buffer, *s++);
    }
    stringPushbackChar(buffer, '\0');
}

String::String(const char *s, uint32_t len) : buffer(1)
{
    uint32_t l = 0;
    while (s && *s && l < len)
    {
        stringPushbackChar(buffer, *s++);
        ++l;
    }
    stringPushbackChar(buffer, '\0');
}

String::String(const String &s) : buffer(1)
{
    *this = s;
}

String &String::operator=(const String &s)
{
    const char *s2 = s.getStr();
    if (s2 == getStr())
        return *this;
    buffer.clear();
    this->buffer.copyFrom(s.buffer);
    return *this;
}


bool String::operator == (const String &s) const
{
    if(s.getSize() != getSize())
        return false;

    return memcmp(buffer.getBegin(), s.buffer.getBegin(), getSize()) == 0;
}

bool String::operator == (const char *s) const
{
    if(!s)
        return false;

    return memcmp(buffer.getBegin(), s, getSize()) == 0;
}


String &String::append(const String &s)
{
    buffer.reserve(buffer.getSize() + s.getSize());
    buffer.removeIndex(getSize());
    const char *s2 = s.getStr();
    uint32_t addedSize = s.getSize();
    while (addedSize > 0)
    {
        stringPushbackChar(buffer, *s2++);
        --addedSize;
    }
    stringPushbackChar(buffer, '\0');
    return *this;
}

String &String::append(const char *s)
{
    buffer.removeIndex(getSize());
    while (s && *s)
    {
        stringPushbackChar(buffer, *s++);
    }
    stringPushbackChar(buffer, '\0');
    return *this;
}

String &String::append(const char *s, uint32_t charas)
{
    buffer.removeIndex(getSize());
    while (s && *s && charas > 0)
    {
        stringPushbackChar(buffer, *s++);
        --charas;
    }
    stringPushbackChar(buffer, '\0');
    return *this;
}


String &String::append(int64_t i)
{
    char c[32] = {};
    snprintf(c, 32, "%li", i);
    append(c);
    return *this;
}

String &String::append(uint64_t u)
{
    char c[32] = {};
    snprintf(c, 32, "%lu", u);
    append(c);
    return *this;
}

String &String::append(int32_t i)
{
    char c[32] = {};
    snprintf(c, 32, "%i", i);
    append(c);
    return *this;
}

String &String::append(uint32_t u)
{
    char c[32] = {};
    snprintf(c, 32, "%u", u);
    append(c);
    return *this;
}

String &String::append(float f)
{
    char c[32] = {};
    snprintf(c, 32, "%f", f);
    append(c);
    return *this;
}

String &String::append(double d)
{
    char c[32] = {};
    snprintf(c, 32, "%lf", d);
    append(c);
    return *this;
}

String &String::append(uint32_t amount, char c)
{
    ASSERT(amount < 32);
    char arr[32] = {};
    for(;amount > 0; --amount)
        arr[amount - 1] = c;
    append(arr);
    return *this;
}

char &String::operator[] (uint32_t index) const
{
    ASSERT(index < getSize() + 1);
    if(index > getSize())
        return (char &) (*(buffer.getBegin() + getSize()));
    return (char &) (*(buffer.getBegin() + index));
}

void String::clear()
{
    buffer.clear();
    stringPushbackChar(buffer, '\0');
}

