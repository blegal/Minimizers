#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>

class file_writer
{
protected:
    bool is_fopen = false;

public:
             file_writer(){};
    virtual ~file_writer(){};

    virtual bool is_open() = 0;
    virtual int  write  (char* buffer, const int eSize, const int eCount) = 0;
    virtual void close  () = 0;
};
