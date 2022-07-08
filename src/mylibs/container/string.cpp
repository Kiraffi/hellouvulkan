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


void String::add(const String &s)
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
}

void String::add(const char *s)
{
    buffer.removeIndex(getSize());
    while (s && *s)
    {
        stringPushbackChar(buffer, *s++);
    }
    stringPushbackChar(buffer, '\0');
}

void String::add(const char *s, uint32_t charas)
{
    buffer.removeIndex(getSize());
    while (s && *s && charas > 0)
    {
        stringPushbackChar(buffer, *s++);
        --charas;
    }
    stringPushbackChar(buffer, '\0');
}


void String::add(int64_t i)
{
    char c[32];
    snprintf(c, 32, "%lli", i);
    add(c);
}

void String::add(uint64_t u)
{
    char c[32];
    snprintf(c, 32, "%llu", u);
    add(c);
}

void String::add(int32_t i)
{
    char c[32];
    snprintf(c, 32, "%i", i);
    add(c);
}

void String::add(uint32_t u)
{
    char c[32];
    snprintf(c, 32, "%u", u);
    add(c);
}

void String::add(float f)
{
    char c[32];
    snprintf(c, 32, "%f", f);
    add(c);
}

void String::add(double d)
{
    char c[32];
    snprintf(c, 32, "%lf", d);
    add(c);
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

