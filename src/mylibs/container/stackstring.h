#pragma once

#include <core/assert.h>
#include <core/mytypes.h>
#include <core/general.h>

template<u32 N>
class StackString
{
public:
    StackString() {}
    StackString(const char *s)
    {
        while (s && *s && size < MaxSize)
        {
            str[size++] = *s++;
        }
        str[size] = '\0';
    }
    StackString(const char *s, u32 len)
    {
        while(s && *s && size < MaxSize && size < len)
        {
            str[size++] = *s++;
        }
        str[size] = '\0';
    }

    template<u32 U>
    StackString(const StackString<U> &s)
    {
        const char *s2 = s.getStr();
        if (s2 == str)
            return;
        while (s2 && *s2 && size < MaxSize)
        {
            str[size++] = *s2++;
        }
        str[size] = '\0';
    }

    template<u32 U>
    void add(const StackString<U> &s)
    {
        const char *s2 = s.getStr();
        while (s2 && *s2 && size < MaxSize)
        {
            str[size++] = *s2++;
        }
        str[size] = '\0';
    }

    template<u32 U>
    bool operator == (const StackString<U> &s) const
    {
        if(s.size != size)
            return false;

        return Supa::memcmp(s.str, str, size) == 0;
    }

    bool operator == (const char *s) const
    {
        if(!s)
            return false;

        for(u32 i = 0; i < size; ++i)
        {
            if(s[i] != str[i])
                return false;
        }
        return true;
    }


    // buff len must include space for 0 character terminator
    void copyToCharStr(char *outStr, u32 buffLenWithZeroTerminator)
    {
        u32 charCount = size < buffLenWithZeroTerminator - 1 ? size : buffLenWithZeroTerminator - 1;
        if(charCount > 0)
        {
            Supa::memcpy(outStr, str, charCount);
            outStr[charCount] = '\0';
        }
    }
    void add(const char *s)
    {
        while (s && *s && size < MaxSize)
        {
            str[size] = (*s++);
            ++size;
        }
        str[size] = '\0';
    }

    void add(const char *s, u32 charas)
    {
        u32 addedCharacters = 0u;
        while (s && *s && size < MaxSize && addedCharacters < charas)
        {
            str[size] = (*s++);
            ++size;
            ++addedCharacters;
        }
        str[size] = '\0';
    }


    void add(i64 i)
    {
        char c[32];
        snprintf(c, 32, "%" PRIi64, i);
        u32 size2 = 0u;
        while (size2 < 32 && c[size2] && size < MaxSize)
        {
            str[size] = c[size2];
            ++size;
            ++size2;
        }
        str[size] = '\0';
    }

    void add(u64 u)
    {
        char c[32];
        snprintf(c, 32, "%" PRIu64, u);
        u32 size2 = 0u;
        while (size2 < 32 && c[size2] && size < MaxSize)
        {
            str[size] = c[size2];
            ++size;
            ++size2;
        }
        str[size] = '\0';
    }

    void add(i32 i)
    {
        char c[32];
        snprintf(c, 32, "%i", i);
        u32 size2 = 0u;
        while (size2 < 32 && c[size2] && size < MaxSize)
        {
            str[size] = c[size2];
            ++size;
            ++size2;
        }
        str[size] = '\0';
    }

    void add(u32 u)
    {
        char c[32];
        snprintf(c, 32, "%u", u);
        u32 size2 = 0u;
        while (size2 < 32 && c[size2] && size < MaxSize)
        {
            str[size] = c[size2];
            ++size;
            ++size2;
        }
        str[size] = '\0';
    }

    void add(float f)
    {
        char c[32];
        snprintf(c, 32, "%f", f);
        u32 size2 = 0u;
        while (size2 < 32 && c[size2] && size < MaxSize)
        {
            str[size] = c[size2];
            ++size;
            ++size2;
        }
        str[size] = '\0';
    }
    char &operator[] (u32 index) const
    {
        ASSERT(index < size);
        if(index > size)
            return (char &) (*str);
        return (char &) (*(str + index));
    }

    void clear()
    {
        size = 0;
        str[size] = '\0';
    }

    bool empty() const { return size == 0; }
    const char *getStr() const { return str; }
    u32 getSize() const { return size; }
    u32 length() const { return size; }
private:
    char str[N + 1] = {};
    static constexpr u32 MaxSize = N;
    u16 size = 0u;
};

using TinyStackString = StackString<21>;    // 24 bytes
using SmallStackString = StackString<29>;   // 32 bytes
using MediumStackString = StackString<253>; // 256 bytes
using LongStackString = StackString<1021>;  // 1024 bytes
