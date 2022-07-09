#include "file.h"

#include <core/general.h>
#include <core/mytypes.h>

#include <filesystem>
#include <fstream>

/*
// for GetCurrentDirectory
#ifdef WIN32
#include <Windows.h>
#endif
*/

bool loadBytes(const char *filename, ByteBuffer &dataOut)
{
    /*
#ifdef WIN32
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


        f.read((char *)dataOut.getBegin(), s);

        //printf("filesize: %u\n", s);
        return true;
    }
    return false;
}


bool writeBytes(const char *filename, const ByteBuffer &data)
{
    //
    //if(std::filesystem::exists(filename))
    {
        std::filesystem::path p(filename);


        std::ofstream f(p, std::ios::out | std::ios::binary);

        printf("Writing bytes: %u to :%s\n", data.getSize(), filename);
        bool success = !(f.write((const char *)data.getBegin(), data.getSize())).fail();
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

