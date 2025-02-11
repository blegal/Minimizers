#pragma once
#include "../../file_reader.hpp"
#include <zlib.h>

class file_gz_reader : public file_reader
{
private:
    gzFile  gzfp;

public:
     file_gz_reader(const std::string& filen);
     file_gz_reader(const char*       filen);
    ~file_gz_reader();

    virtual bool is_open();
    virtual void close  ();
    virtual bool is_eof ();
    virtual int  read   (char* buffer, int eSize, int eCount);
};
