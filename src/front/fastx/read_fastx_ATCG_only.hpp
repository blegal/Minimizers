#pragma once
#include "../file_reader.hpp"

class read_fastx_ATCG_only : public file_reader
{
private:
    const int buff_size = 64 * 1024 * 1024; // taille du tampon entre le fichier et l'application
    char* buffer;
    int n_data   = 0;
    int c_ptr    = 0;
    int n_qualities = 0;
    int n_lines        = 0;
    bool no_more_load  = false;
    bool file_ended    = false;
    FILE* f;
    long long int debug = 0;

public:
     read_fastx_ATCG_only(const std::string filen);
    ~read_fastx_ATCG_only();

    virtual bool next_sequence(char* n_kmer);

    virtual std::tuple<int, bool> next_sequence(char* n_kmer, int buffer_size, const bool _internal_ = false);

    virtual bool reload();

    virtual bool is_eof();
};
