#include "file.h"

#include <core/assert.h>
#include <core/general.h>
#include <core/mytypes.h>

//#include <filesystem>
//#include <fstream>

#include <stdio.h>
#include <stdlib.h>


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
        //std::filesystem::path p(filename);
        //uint32_t t = uint32_t(std::filesystem::file_size(p));
        #if WIN32
            FILE *fp = nullptr;
            fopen_s(&fp, filename, "rb");
        #else

            FILE *fp = fopen(filename, "rb");
        #endif
        if(!fp)
            return false;
        uint32_t s = 0;

        while(fgetc(fp) != EOF)
            ++s;
        rewind(fp);
        dataOut.resize(s);

        uint8_t *ptr = dataOut.getBegin();
        int c;
        while((c = fgetc(fp)) != EOF)
            *ptr++ = c;
        fclose(fp);
        //std::ifstream f(p, std::ios::in | std::ios::binary);
        //f.read((char *)newBuffer.getBegin(), s);
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
        printf("Writing bytes: %u to :%s\n", data.getSize(), filename);

        //std::filesystem::path p(filename);
        //std::ofstream f(p, std::ios::out | std::ios::binary);
        //bool success = !(f.write((const char *)data.getBegin(), data.getSize())).fail();
        #if WIN32
            FILE *fp = nullptr;
            fopen_s(&fp, filename, "wb");
        #else

            FILE *fp = fopen(filename, "wb");
        #endif
        if(!fp)
            return false;

        size_t bytesWritten = fwrite(data.getBegin(), 1, data.getSize(), fp);
        bool success = bytesWritten != data.getSize();
        fclose(fp);
        if(!success)
        {
            printf("failed to write file\n");
        }
        return success;
    }
    //return false;
}


bool fileExists(const char *filename)
{
#if WIN32
    FILE *fp = nullptr;
    fopen_s(&fp, filename, "rb");
#else
    FILE *fp = fopen(filename, "rb");
#endif
    bool result = fp != nullptr;
    if(fp)
        fclose(fp);
    return result;


    //return std::filesystem::exists(fileName);
}

