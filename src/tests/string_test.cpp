#include <container/string.h>
#include <core/assert.h>

#include <stdio.h>
#include <string.h>

static void testStringValidness(const String &s, const char *str)
{
    uint32_t len = strlen(str);
    ASSERT(s.length() == len);
    ASSERT(s.getStr() != nullptr);
    ASSERT(memcmp(s.getStr(), str, len + 1) == 0);
}

static void testStringValidnessFloat(const String &s, const char *str)
{
    uint32_t len = strlen(str);
    uint32_t len2 = s.length();
    ASSERT(len <= len2);
    uint32_t i = 0;
    for(; i < len; ++i)
    {
        ASSERT(s[i] == str[i]);
    }
    for(; i < len2; ++i)
    {
        ASSERT(s[i] == '0');
    }

    ASSERT(s.getStr()[i] == '\0');
}

static void testStringConstructors()
{
    String s1;
    printf("%s\n", s1.getStr());
    testStringValidness(s1, "\0");

    String s2("Foo");
    printf("%s\n", s2.getStr());
    testStringValidness(s2, "Foo\0");

    String s3(s2);
    printf("%s\n", s3.getStr());
    testStringValidness(s3, "Foo\0");

    String s4 = s2;
    printf("%s\n", s4.getStr());
    testStringValidness(s4, "Foo\0");

    String s5("Foobar", 2);
    printf("%s\n", s5.getStr());
    testStringValidness(s5, "Fo\0");
}

static void testStringAdd()
{
    String s1("Foo");
    printf("%s\n", s1.getStr());
    testStringValidness(s1, "Foo\0");

    String s2("Bar");
    printf("%s\n", s2.getStr());
    testStringValidness(s2, "Bar\0");

    String s3;
    s3.add(s1);
    s3.add(s2);
    printf("%s\n", s3.getStr());
    testStringValidness(s3, "FooBar\0");

    String s4;
    s4.add(2);
    printf("%s\n", s4.getStr());
    testStringValidness(s4, "2\0");

    String s6;
    s6.add(-1);
    printf("%s\n", s6.getStr());
    testStringValidness(s6, "-1\0");

    String s7;
    s7.add(1.0);
    printf("%s\n", s7.getStr());
    testStringValidnessFloat(s7, "1.0\0");

}

static void testStringOther()
{
    const char *str = "abcAB";
    String s1(str);
    printf("%s\n", s1.getStr());
    ASSERT(s1[3] == 'A');
    ASSERT(s1[0] == 'a');
    {
        uint32_t ind = 0;
        for(const auto c : s1)
            ASSERT(c == str[ind++]);
    }
    s1.clear();
    printf("%s\n", s1.getStr());
    testStringValidnessFloat(s1, "\0");
    testStringValidness(s1, "\0");


}

void testStrings()
{
    testStringConstructors();
    testStringOther();
    testStringAdd();
}
