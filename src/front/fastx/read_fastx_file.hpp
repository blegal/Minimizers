#pragma once
#include "../file_reader.hpp"

class read_fastx_file : public file_reader
{
private:
    const int buff_size = 2 * 1024 * 1024; // taille du tampon entre le fichier et l'application
    char* buffer;
    int n_data   = 0;
    int c_ptr    = 0;
    int n_lines        = 0;
    bool no_more_load  = false;
    bool file_ended    = false;
    FILE* f;

public:
     read_fastx_file(const std::string filen);
    ~read_fastx_file();

    virtual bool next_sequence(char* n_kmer);

    virtual std::tuple<int, bool> next_sequence(char* n_kmer, int buffer_size, const bool _internal_ = false);

    virtual bool reload();

    virtual bool is_eof();
};
