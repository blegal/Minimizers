#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>

class file_reader
{
protected:
    bool is_fopen = false;
    bool is_foef  = false;

public:
    file_reader(){};

    virtual ~file_reader(){};

    virtual bool is_open () = 0;
    virtual void close  ();
    virtual bool is_eof  () = 0;
    virtual int  read    (char* buffer, int eSize, int eCount) = 0;
};
