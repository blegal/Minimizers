#pragma once
#include "../../file_reader.hpp"
#include "../../../front/fastx_lz4/lz4/lz4file.h"

class file_raw_reader : public file_reader
{
private:
    FILE*             stream;

public:
     file_raw_reader(const std::string& filen);
     file_raw_reader(const char*       filen);
    ~file_raw_reader();

    virtual bool is_open();
    virtual void close  ();
    virtual bool is_eof ();
    virtual int  read   (char* buffer, int eSize, int eCount);
};
