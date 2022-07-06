#pragma once
#include <stdint.h>

class StringView
{
public:
    StringView() : ptr(nullptr), length(0) {}
    StringView(const char *s);
    StringView(const char *s, uint32_t len);
    
    const char *const begin() const { return ptr; }
    const char *const end() const { return ptr + length; }

    const char &operator[] (uint32_t index) const;
    bool operator==(const char *a) const;
    bool operator==(StringView other) const;


    bool empty() const { return length == 0 || ptr == nullptr; }
    bool isValid() const { return (ptr != nullptr && length > 0u) || (ptr == nullptr && length == 0); }

    uint32_t size() const { return length; }
    const char *const data() const { return ptr; }
    const char *ptr = nullptr;
    uint32_t length = 0u;
};
