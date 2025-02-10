#pragma once
#include "../../file_writer.hpp"
#include "../../../front/fastx_lz4/lz4/lz4file.h"

class file_lz4_writer : public file_writer
{
private:
    FILE*             stream;
    LZ4_writeFile_t* lz4fWrite;

public:
     file_lz4_writer(const std::string& filen);
     file_lz4_writer(const char*       filen);
    ~file_lz4_writer();

    virtual bool is_open();
    virtual int  write  (char* buffer, int eSize, int eCount);
    virtual void close  ();
};
