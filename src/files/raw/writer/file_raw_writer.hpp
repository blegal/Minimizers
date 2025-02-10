#pragma once
#include "../../file_writer.hpp"

class file_raw_writer : public file_writer
{
private:
    FILE*             stream;

public:
     file_raw_writer(const std::string& filen);
     file_raw_writer(const char*       filen);
    ~file_raw_writer();

    virtual bool is_open ();
    virtual int  write  (char* buffer, int eSize, int eCount);
    virtual void close  ();
};
