#pragma once
#include "../file_reader.hpp"
#include "../../front/fastx_lz4/lz4/lz4file.h"

class file_lz4_reader : public file_reader
{
private:
    FILE*             stream;
    LZ4_readFile_t* lz4fRead;
    bool         stream_open;

public:
     file_lz4_reader(const std::string& filen);
     file_lz4_reader(const char*       filen);
    ~file_lz4_reader();

    virtual bool isOpen ();
    virtual bool isClose();
    virtual bool is_eof ();
    virtual int  read   (char* buffer, int eSize, int eCount);
};
