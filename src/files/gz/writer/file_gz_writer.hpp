#pragma once
#include "../../file_writer.hpp"
#include <zlib.h>

class file_gz_writer : public file_writer
{
private:
    gzFile gzfp;

public:
     file_gz_writer(const std::string& filen);
     file_gz_writer(const char*        filen);
    ~file_gz_writer();

    virtual bool is_open ();
    virtual int  write  (char* buffer, int eSize, int eCount);
    virtual void close  ();
};
