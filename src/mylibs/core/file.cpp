#include "file.h"

#include <core/general.h>
#include <core/mytypes.h>

#include <filesystem>
#include <fstream>
//#include <math.h>
//#include <stdio.h>
//#include <string.h>

/* 
// for GetCurrentDirectory
#ifdef _WIN32
#include <Windows.h>
#endif
*/

bool loadBytes(const char *filename, PodVector<char> &dataOut)
{
    /*
#ifdef _WIN32
    char buf[1024] = {};
    GetCurrentDirectory(1024, buf);
    //LOG("Buf: %s\n", buf);
#endif
*/
    if(fileExists(filename))
    {
        std::filesystem::path p(filename);
        uint32_t s = uint32_t(std::filesystem::file_size(p));

        dataOut.resize(s);

        std::ifstream f(p, std::ios::in | std::ios::binary);


        f.read(dataOut.data(), s);

        //printf("filesize: %u\n", s);
        return true;
    }
    return false;
}


bool writeBytes(const char *filename, ArraySliceViewBytes bytes)
{
    //
    //if(std::filesystem::exists(filename))
    {
        std::filesystem::path p(filename);


        std::ofstream f(p, std::ios::out | std::ios::binary);

        printf("Writing bytes: %u to :%s\n", uint32_t(bytes.size()), filename);
        bool success = !(f.write((const char *)bytes.data(), bytes.size())).fail();
        if(!success)
        {
            printf("failed to write file\n");
        }
        return success;
    }
    //return false;
}


bool fileExists(const char *fileName)
{
    return std::filesystem::exists(fileName);
}

