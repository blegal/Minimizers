#pragma once
#include "../../stream_writer.hpp"
#include "../../../front/fastx_lz4/lz4/lz4file.h"

class stream_lz4_writer : public stream_writer
{
private:
    FILE*             stream;
    LZ4_writeFile_t* lz4fWrite;

public:
     stream_lz4_writer(const std::string& filen);
     stream_lz4_writer(const char*       filen);
    ~stream_lz4_writer();

    virtual bool is_open();
    virtual int  write  (void* buffer, int eSize, int eCount);
    virtual void close  ();
};
