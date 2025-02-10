#pragma once
#include <bzlib.h>
#include "../../file_writer.hpp"

class file_bz2_writer : public file_writer
{
private:
    FILE*    stream;
    BZFILE*  bz2fWrite;

public:
     file_bz2_writer(const std::string& filen);
     file_bz2_writer(const char*        filen);
    ~file_bz2_writer();

    virtual bool is_open ();
    virtual void close  ();
    virtual int  write  (char* buffer, int eSize, int eCount);
};
