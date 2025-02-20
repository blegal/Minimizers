#pragma once
#include "../../stream_reader.hpp"
#include <zlib.h>

class stream_gz_reader : public stream_reader
{
private:
    gzFile  gzfp;

public:
     stream_gz_reader(const std::string& filen);
     stream_gz_reader(const char*       filen);
    ~stream_gz_reader();

    virtual bool is_open();
    virtual void close  ();
    virtual bool is_eof ();
    virtual int  read   (char* buffer, int eSize, int eCount);
};
