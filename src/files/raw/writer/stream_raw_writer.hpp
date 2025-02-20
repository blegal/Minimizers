#pragma once
#include "../../stream_writer.hpp"

class stream_raw_writer : public stream_writer
{
private:
    FILE*             stream;

public:
     stream_raw_writer(const std::string& filen);
     stream_raw_writer(const char*       filen);
    ~stream_raw_writer();

    virtual bool is_open ();
    virtual int  write  (char* buffer, int eSize, int eCount);
    virtual void close  ();
};
