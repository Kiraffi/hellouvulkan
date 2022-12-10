#pragma once

#include <container/bytebuffer.h>
#include <core/mytypes.h>

class String
{
public:
    String();
    virtual ~String() { buffer.~ByteBuffer(); }
    String(const char *s);
    String(const char *s, u32 len);
    String(const String &s);
    String &operator =(const String &s);

    bool operator == (const String &s) const;
    bool operator == (const char *s) const;

    String &append(const String &s);
    String &append(const char *s);
    String &append(const char *s, u32 charas);
    String &append(char c);
    String &append(i64 i);
    String &append(u64 u);
    String &append(i32 i);
    String &append(u32 u);
    String &append(float f);
    String &append(double d);
    String &append(u32 amount, char c);

    const char* begin() const { return (const char *)(buffer.getBegin()); }
    const char* data() const  { return (const char *)(buffer.getBegin()); }
    const char* end() const   { return begin() + getSize(); }

    char* begin() { return (char *)(buffer.getBegin()); }
    char* data()  { return (char *)(buffer.getBegin()); }
    char* end()   { return begin() + getSize(); }


    char &operator[] (u32 index) const;
    void clear();
    void erase(u32 index) { buffer.removeIndex(index); }
    bool empty() const { return buffer.getSize() == 0; }

    void reserve(u32 amount) { buffer.reserve(amount); }
    void resize(u32 amount);
    const char *getStr() const { return (const char *)buffer.getBegin(); }
    u32 getSize() const { return buffer.getSize() - 1; }
    u32 length() const { return buffer.getSize() - 1; }
    u32 size() const { return buffer.getSize() - 1; }

    ByteBuffer &getBuffer() { return buffer; }
    const ByteBuffer &getBuffer() const { return buffer; }

private:
    ByteBuffer buffer;
};
