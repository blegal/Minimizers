#pragma once
#include "../file_reader.hpp"
#include <zlib.h>

class file_gz_reader : public file_reader
{
private:
    FILE*   stream;
    gzFile  streaz;
    bool         stream_open;

public:
     file_gz_reader(const std::string& filen);
     file_gz_reader(const char*       filen);
    ~file_gz_reader();

    virtual bool isOpen () = 0;
    virtual bool isClose() = 0;
    virtual bool is_eof()  = 0;
    virtual int  read   (char* buffer, int eSize, int eCount) = 0;
};
