#include "general.h"
#include "mytypes.h"
#include <filesystem>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <Windows.h>
#endif


float fsqrtf(float a) { return sqrtf(a); }
float ftanf(float a) { return tanf(a); }
float fsinf(float a) { return sinf(a); }
float fcosf(float a) { return cosf(a); }
float ffabsf(float a) { uint32_t b = *((uint32_t *)(&a)); b &= 0x7fff'ffff; return *((float *)(&b)); }
float ffmodf(float a, float b) { return fmodf(a, b); }
float ffminf(float a, float b) { return (a < b) ? a : b; }
float ffmaxf(float a, float b) { return (a > b) ? a : b; }

//void myPrint(const char* ptr, )

bool loadBytes(std::string_view fileName, std::vector<char>& dataOut)
{
    #ifdef _WIN32
        char buf[1024] = {};
        GetCurrentDirectory(1024, buf);
        LOG("Buf: %s\n", buf);
    #endif

    if (std::filesystem::exists(fileName))
    {
        std::filesystem::path p(fileName);
        uint32_t s = uint32_t(std::filesystem::file_size(p));

        dataOut.resize(s);

        std::ifstream f(p, std::ios::in | std::ios::binary);


        f.read(dataOut.data(), s);

        printf("filesize: %u\n", s);
        return true;
    }
    return false;
}




// color values r,g,h,a between [0..1]
uint32_t getColor(float r, float g, float b, float a)
{
    r = fmaxf(0.0f, fminf(r, 1.0f));
    g = fmaxf(0.0f, fminf(g, 1.0f));
    b = fmaxf(0.0f, fminf(b, 1.0f));
    a = fmaxf(0.0f, fminf(a, 1.0f));

    uint32_t c = 0u;
    c += (uint32_t(r * 255.0f) & 255u);
    c += (uint32_t(g * 255.0f) & 255u) << 8u;
    c += (uint32_t(b * 255.0f) & 255u) << 16u;
    c += (uint32_t(a * 255.0f) & 255u) << 24u;

    return c;
}
