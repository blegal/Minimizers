#pragma once
#include "../../stream_writer.hpp"
#include <zlib.h>

class stream_gz_writer : public stream_writer
{
private:
    gzFile gzfp;

public:
     stream_gz_writer(const std::string& filen);
     stream_gz_writer(const char*        filen);
    ~stream_gz_writer();

    virtual bool is_open ();
    virtual int  write  (void* buffer, int eSize, int eCount);
    virtual void close  ();
};
