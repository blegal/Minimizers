#pragma once
#include "../../stream_reader.hpp"
#include "../../../front/fastx_lz4/lz4/lz4file.h"

class stream_lz4_reader : public stream_reader
{
private:
    FILE*             stream;
    LZ4_readFile_t* lz4fRead;

public:
     stream_lz4_reader(const std::string& filen);
     stream_lz4_reader(const char*       filen);
    ~stream_lz4_reader();

    virtual bool is_open();
    virtual void close  ();
    virtual bool is_eof ();
    virtual int  read   (char* buffer, int eSize, int eCount);
};
