#pragma once

#include <container/bytebuffer.h>
#include <core/mytypes.h>

class String
{
public:
    String();
    virtual ~String() {}
    String(const char *s);
    String(const char *s, uint32_t len);
    String(const String &s);
    String &operator =(const String &s);

    bool operator == (const String &s) const;
    bool operator == (const char *s) const;

    void add(const String &s);
    void add(const char *s);
    void add(const char *s, uint32_t charas);
    void add(int64_t i);
    void add(uint64_t u);
    void add(int32_t i);
    void add(uint32_t u);
    void add(float f);
    void add(double d);

    const char* begin() const { return (const char *)(buffer.getBegin()); }
    const char* data() const  { return (const char *)(buffer.getBegin()); }
    const char* end() const   { return begin() + getSize(); }

    char* begin() { return (char *)(buffer.getBegin()); }
    char* data()  { return (char *)(buffer.getBegin()); }
    char* end()   { return begin() + getSize(); }


    char &operator[] (uint32_t index) const;
    void clear();
    bool empty() const { return buffer.getSize() == 0; }
    const char *getStr() const { return (const char *)buffer.getBegin(); }
    uint32_t getSize() const { return buffer.getSize() - 1; }
    uint32_t length() const { return buffer.getSize() - 1; }

private:
    ByteBuffer buffer;
};
