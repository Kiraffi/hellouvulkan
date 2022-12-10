#include "string.h"

#include <core/assert.h>
#include <core/general.h>
#include <core/mytypes.h>

static void stringPushbackChar(ByteBuffer &buffer, char c)
{
    buffer.insertIndex(buffer.getSize(), (u8 *)(&c));
}

String::String() : buffer(1, BufferType::STRING)
{
    buffer.clear();
    stringPushbackChar(buffer, '\0');
}

String::String(const char *s) : buffer(1, BufferType::STRING)
{
    while (s && *s)
    {
        stringPushbackChar(buffer, *s++);
    }
    stringPushbackChar(buffer, '\0');
}

String::String(const char *s, u32 len) : buffer(1, BufferType::STRING)
{
    u32 l = 0;
    while (s && *s && l < len)
    {
        stringPushbackChar(buffer, *s++);
        ++l;
    }
    stringPushbackChar(buffer, '\0');
}

String::String(const String &s) : buffer(1, BufferType::STRING)
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

    return Supa::memcmp(buffer.getBegin(), s.buffer.getBegin(), getSize()) == 0;
}

bool String::operator == (const char *s) const
{
    if(!s)
        return false;

    return Supa::memcmp(buffer.getBegin(), s, getSize()) == 0;
}


String &String::append(const String &s)
{
    buffer.reserve(buffer.getSize() + s.getSize());
    buffer.removeIndex(getSize());
    const char *s2 = s.getStr();
    u32 addedSize = s.getSize();
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

String &String::append(const char *s, u32 charas)
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

String &String::append(char c)
{
    buffer.removeIndex(getSize());
    stringPushbackChar(buffer, c);
    stringPushbackChar(buffer, '\0');
    return *this;
}

String &String::append(i64 i)
{
    char c[32] = {};
#if WIN32
    snprintf(c, 32, "%lli", i);
#else
    snprintf(c, 32, "%li", i);
#endif
    append(c);
    return *this;
}

String &String::append(u64 u)
{
    char c[32] = {};
#if WIN32
    snprintf(c, 32, "%llu", u);
#else
    snprintf(c, 32, "%lu", u);
#endif
    append(c);
    return *this;
}

String &String::append(i32 i)
{
    char c[32] = {};
    snprintf(c, 32, "%i", i);
    append(c);
    return *this;
}

String &String::append(u32 u)
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

String &String::append(u32 amount, char c)
{
    ASSERT(amount < 32);
    char arr[32] = {};
    for(;amount > 0; --amount)
        arr[amount - 1] = c;
    append(arr);
    return *this;
}

char &String::operator[] (u32 index) const
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

void String::resize(u32 amount)
{
    buffer.resize(amount + 1);
    buffer.getBegin()[amount] = '\0';
}
