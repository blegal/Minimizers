#pragma once
#include <bzlib.h>
#include "../../stream_writer.hpp"

class stream_bz2_writer : public stream_writer
{
private:
    FILE*    stream;
    BZFILE*  bz2fWrite;

public:
     stream_bz2_writer(const std::string& filen);
     stream_bz2_writer(const char*        filen);
    ~stream_bz2_writer();

    virtual bool is_open ();
    virtual void close  ();
    virtual int  write  (char* buffer, int eSize, int eCount);
};
