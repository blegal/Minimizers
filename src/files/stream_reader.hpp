#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>

class stream_reader
{
protected:
    bool is_fopen = false;
    bool is_foef  = false;

public:
    stream_reader(){};

    virtual ~stream_reader(){};

    virtual bool is_open () = 0;
    virtual void close   () = 0;
    virtual bool is_eof  () = 0;
    virtual int  read    (char* buffer, int eSize, int eCount) = 0;
};
