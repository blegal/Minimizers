#pragma once
#include "../../file_reader.hpp"
#include <bzlib.h>

class file_bz2_reader : public file_reader
{
private:
    BZFILE* bzf;

public:
     file_bz2_reader(const std::string& filen);
     file_bz2_reader(const char*       filen);
    ~file_bz2_reader();

    virtual bool is_open();
    virtual void close  ();
    virtual bool is_eof ();
    virtual int  read   (char* buffer, int eSize, int eCount);
};
