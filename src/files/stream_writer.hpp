#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>

class stream_writer
{
protected:
    bool is_fopen = false;

public:
             stream_writer(){};
    virtual ~stream_writer(){};

    virtual bool is_open() = 0;
    virtual int  write  (void* buffer, const int eSize, const int eCount) = 0;
    virtual void close  () = 0;
};
