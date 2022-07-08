#include "stringview.h"

#include <container/string.h>

#include <core/assert.h>
#include <core/mytypes.h>

StringView::StringView(const char *s)
{
    if(!s)
    {
        length = 0;
        ptr = nullptr;
    }
    else
    {
        ptr = s;
        while(*s++ != '\0')
            ++length;
    }
}

StringView::StringView(const char *s, uint32_t len)
{
    if(!s)
    {
        length = 0;
        ptr = nullptr;
    }
    ptr = s;
    while(*s++ != '\0' && length < len)
        ++length;
}

StringView::StringView(const String &s)
{
    const char *b = s.begin();
    ptr = b;

    while(*b++ != '\0' && length < s.getSize())
        ++length;
}


const char &StringView::operator[] (uint32_t index) const
{
    ASSERT(index < length);
    ASSERT(ptr);
    return ptr[index];
}

bool StringView::operator==(const char *s) const
{
    if(!s)
        return false;
    const char *p = ptr;
    for(uint32_t i = 0; i < length; ++i)
    {
        if(*s++ != *p++)
            return false;
    }
    return true;
}

bool StringView::operator==(StringView other) const
{
    if(other.length != length)
        return false;
    if(ptr == other.ptr)
        return true;
    return memcmp(ptr, other.ptr, length) == 0;
}
