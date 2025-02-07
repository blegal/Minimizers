#pragma once
#include "../file_reader.hpp"
#include <bzlib.h>

class file_bz2_reader : public file_reader
{
private:
    FILE*   stream;
    BZFILE* streaz;
    bool    stream_open;

public:
     file_bz2_reader(const std::string& filen);
     file_bz2_reader(const char*       filen);
    ~file_bz2_reader();

    virtual bool isOpen () = 0;
    virtual bool isClose() = 0;
    virtual bool is_eof()  = 0;
    virtual int  read   (char* buffer, int eSize, int eCount) = 0;
};
