#include <container/string.h>
#include <core/assert.h>
#include <core/general.h>

static void testStringValidness(const String &s, const char *str)
{
    uint32_t len = Supa::strlen(str);
    ASSERT(s.length() == len);
    ASSERT(s.getStr() != nullptr);
    ASSERT(Supa::memcmp(s.getStr(), str, len + 1) == 0);
}

static void testStringValidnessFloat(const String &s, const char *str)
{
    uint32_t len = Supa::strlen(str);
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

    String s3("Fii");
    s3.append(s1);
    s3.append(s2);
    s3.append("Pooopaaa");
    printf("%s\n", s3.getStr());
    testStringValidness(s3, "FiiFooBarPooopaaa\0");

    String s4;
    s4.append(2);
    printf("%s\n", s4.getStr());
    testStringValidness(s4, "2\0");

    String s6;
    s6.append(-1);
    printf("%s\n", s6.getStr());
    testStringValidness(s6, "-1\0");

    String s7;
    s7.append(1.0);
    printf("%s\n", s7.getStr());
    testStringValidnessFloat(s7, "1.0\0");

    String s8("Fifi");
    s8.append(5, 'a');
    printf("%s\n", s8.getStr());
    testStringValidness(s8, "Fifiaaaaa");

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
