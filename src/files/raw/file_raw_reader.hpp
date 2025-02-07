#pragma once
#include "../file_reader.hpp"
#include "../../front/fastx_lz4/lz4/lz4file.h"

class file_raw_reader : public file_reader
{
private:
    FILE*             stream;

public:
     file_raw_reader(const std::string& filen);
     file_raw_reader(const char*       filen);
    ~file_raw_reader();

    virtual bool isOpen () = 0;
    virtual bool isClose() = 0;
    virtual bool is_eof()  = 0;
    virtual int  read   (char* buffer, int eSize, int eCount) = 0;
};
