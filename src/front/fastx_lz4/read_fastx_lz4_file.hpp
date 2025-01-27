#pragma once
#include "../file_reader.hpp"
#include "lz4/lz4file.h"

class read_fastx_lz4_file : public file_reader
{
private:
    const int buff_size = 67108864; // 64MB = 64 * 1024 * 1024;
    char* buffer;
    int n_data   = 0;
    int c_ptr    = 0;
    int n_lines        = 0;
    bool no_more_load  = false;
    bool file_ended    = false;
    FILE*   stream;
    LZ4_readFile_t* lz4fRead;

public:
     read_fastx_lz4_file(const std::string filen);
    ~read_fastx_lz4_file();

    virtual bool next_sequence(char* n_kmer);

    virtual std::tuple<int, bool> next_sequence(char* n_kmer, int buffer_size, const bool _internal_ = false);

    virtual bool reload();

    virtual bool is_eof();
};
