#pragma once

#include <core/assert.h>
#include <core/mytypes.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

template<uint32_t N>
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
    StackString(const char *s, uint32_t len)
    {
        while(s && *s && size < MaxSize && size < len)
        {
            str[size++] = *s++;
        }
        str[size] = '\0';
    }

    template<uint32_t U>
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

    template<uint32_t U>
    void add(const StackString<U> &s)
    {
        const char *s2 = s.getStr();
        while (s2 && *s2 && size < MaxSize)
        {
            str[size++] = *s2++;
        }
        str[size] = '\0';
    }

    // buff len must include space for 0 character terminator
    void copyToCharStr(char *outStr, uint32_t buffLenWithZeroTerminator)
    {
        uint32_t charCount = size < buffLenWithZeroTerminator - 1 ? size : buffLenWithZeroTerminator - 1;
        if(charCount > 0)
        {
            memcpy(outStr, str, charCount);
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

    void add(const char *s, uint32_t charas)
    {
        uint32_t addedCharacters = 0u;
        while (s && *s && size < MaxSize && addedCharacters < charas)
        {
            str[size] = (*s++);
            ++size;
            ++addedCharacters;
        }
        str[size] = '\0';
    }


    void add(int64_t i)
    {
        char c[32];
        snprintf(c, 32, "%lli", i);
        uint32_t size2 = 0u;
        while (size2 < 32 && c[size2] && size < MaxSize)
        {
            str[size] = c[size2];
            ++size;
            ++size2;
        }
        str[size] = '\0';
    }

    void add(uint64_t u)
    {
        char c[32];
        snprintf(c, 32, "%llu", u);
        uint32_t size2 = 0u;
        while (size2 < 32 && c[size2] && size < MaxSize)
        {
            str[size] = c[size2];
            ++size;
            ++size2;
        }
        str[size] = '\0';
    }

    void add(int32_t i)
    {
        char c[32];
        snprintf(c, 32, "%i", i);
        uint32_t size2 = 0u;
        while (size2 < 32 && c[size2] && size < MaxSize)
        {
            str[size] = c[size2];
            ++size;
            ++size2;
        }
        str[size] = '\0';
    }

    void add(uint32_t u)
    {
        char c[32];
        snprintf(c, 32, "%u", u);
        uint32_t size2 = 0u;
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
        uint32_t size2 = 0u;
        while (size2 < 32 && c[size2] && size < MaxSize)
        {
            str[size] = c[size2];
            ++size;
            ++size2;
        }
        str[size] = '\0';
    }
    char &operator[] (uint32_t index) const
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
    uint32_t getSize() const { return size; }
    uint32_t length() const { return size; }
private:
    char str[N + 1] = {};
    static constexpr uint32_t MaxSize = N;
    uint16_t size = 0u;
};

using TinyStackString = StackString<21>;    // 24 bytes
using SmallStackString = StackString<29>;   // 32 bytes
using MediumStackString = StackString<253>; // 256 bytes
using LongStackString = StackString<1021>;  // 1024 bytes
