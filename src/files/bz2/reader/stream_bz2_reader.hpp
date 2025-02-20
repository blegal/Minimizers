#pragma once
#include "../../stream_reader.hpp"
#include <bzlib.h>

class stream_bz2_reader : public stream_reader
{
private:
    BZFILE* bzf;

public:
     stream_bz2_reader(const std::string& filen);
     stream_bz2_reader(const char*       filen);
    ~stream_bz2_reader();

    virtual bool is_open();
    virtual void close  ();
    virtual bool is_eof ();
    virtual int  read   (char* buffer, int eSize, int eCount);
};
