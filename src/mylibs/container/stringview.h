#pragma once

#include <core/mytypes.h>

class String;

class StringView
{
public:
    StringView() : ptr(nullptr), length(0) {}
    StringView(const char *s);
    StringView(const char *s, u32 len);
    StringView(const String &s);

    const char *const begin() const { return ptr; }
    const char *const end() const { return ptr + length; }

    const char &operator[] (u32 index) const;
    bool operator==(const char *a) const;
    bool operator==(StringView other) const;


    bool empty() const { return length == 0 || ptr == nullptr; }
    bool isValid() const { return (ptr != nullptr && length > 0u) || (ptr == nullptr && length == 0); }

    u32 size() const { return length; }
    const char *const data() const { return ptr; }
    const char *ptr = nullptr;
    u32 length = 0u;
};
