#pragma once
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>

class file_reader
{
public:
    file_reader(){};

    virtual ~file_reader(){};

    virtual bool next_sequence(char* n_kmer) = 0;
    virtual std::tuple<int, bool> next_sequence(char* n_kmer, int buffer_size) = 0;
    virtual bool reload() = 0;
    virtual bool is_eof() = 0;
};
