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

    String &append(const String &s);
    String &append(const char *s);
    String &append(const char *s, uint32_t charas);
    String &append(char c);
    String &append(int64_t i);
    String &append(uint64_t u);
    String &append(int32_t i);
    String &append(uint32_t u);
    String &append(float f);
    String &append(double d);
    String &append(uint32_t amount, char c);

    const char* begin() const { return (const char *)(buffer.getBegin()); }
    const char* data() const  { return (const char *)(buffer.getBegin()); }
    const char* end() const   { return begin() + getSize(); }

    char* begin() { return (char *)(buffer.getBegin()); }
    char* data()  { return (char *)(buffer.getBegin()); }
    char* end()   { return begin() + getSize(); }


    char &operator[] (uint32_t index) const;
    void clear();
    void erase(uint32_t index) { buffer.removeIndex(index); }
    bool empty() const { return buffer.getSize() == 0; }

    void reserve(uint32_t amount) { buffer.reserve(amount); }
    void resize(uint32_t amount);
    const char *getStr() const { return (const char *)buffer.getBegin(); }
    uint32_t getSize() const { return buffer.getSize() - 1; }
    uint32_t length() const { return buffer.getSize() - 1; }
    uint32_t size() const { return buffer.getSize() - 1; }


private:
    ByteBuffer buffer;
};
