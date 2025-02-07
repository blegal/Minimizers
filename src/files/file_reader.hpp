#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>

class file_reader
{
protected:
    bool is_open = false;
    bool is_oef  = false;

public:
    file_reader(){};

    virtual ~file_reader(){};

    virtual bool isOpen () = 0;
    virtual bool isClose() = 0;
    virtual bool is_eof()  = 0;
    virtual int  read   (char* buffer, int eSize, int eCount) = 0;
};
