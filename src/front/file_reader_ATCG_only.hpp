#pragma once
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <zlib.h>
#include <cstdint>

class file_reader_ATCG_only
{
public:
    file_reader_ATCG_only(){};

    virtual ~file_reader_ATCG_only(){};

    virtual std::tuple<bool, bool> load_next_chunk(char** out_ptr, uint64_t* out_size) = 0;
};
