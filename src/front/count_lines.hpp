#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <omp.h>

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int count_lines(std::string filename)
{
    printf("(II) Counting the number of sequences (original)\n");
    std::ifstream ifile( filename );

    if( ifile.is_open() == false )
    {
        printf("(EE) File does not exist !\n");
        return EXIT_FAILURE;
    }
    double start_time = omp_get_wtime();
    int n_sequences = 0;
    std::vector<uint64_t> list_hash;
    std::string line, seq;
    while( ifile.eof() == false ) {
        getline(ifile, line);
        n_sequences += 1;
    }
    double end_time = omp_get_wtime();
    ifile.close();
    printf("(II) - %d lines in file\n", n_sequences);
    printf("(II) - It took %g seconds\n", end_time - start_time);
    return n_sequences;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

uint64_t count_lines_c(std::string filename)
{
//    printf("(II) Counting the number of sequences (c)\n");
    FILE* f = fopen( filename.c_str(), "r" );
    if( f == NULL )
    {
//        printf("(EE) File does not exist !\n");
        return EXIT_FAILURE;
    }

    uint64_t n_sequences = 0;
    char buffer[1024 * 1024];
//    double start_time = omp_get_wtime();
    while ( !feof(f) )
    {
        int n = fread(buffer, sizeof(char), 1024 * 1024, f);
        for(int x = 0; x < n; x += 1)
            n_sequences += (buffer[x] == '\n');
    }
    fclose( f );
//    double end_time = omp_get_wtime();
//    printf("(II) - %llu lines in file\n", n_sequences);
//    printf("(II) - It took %g seconds\n", end_time - start_time);
    return n_sequences;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

int count_lines_cpp(std::string filename)
{
    printf("(II) Counting the number of sequences (cpp)\n");
    std::ifstream ifile( filename );
    if( ifile.is_open() == false )
    {
        printf("(EE) File does not exist !\n");
        return EXIT_FAILURE;
    }

    int n_sequences = 0;
    std::vector<char> buffer;
    double start_time = omp_get_wtime();
    buffer.resize(1024 * 1024); // buffer of 1MB size
    while (ifile)
    {
        ifile.read( buffer.data(), buffer.size() );
        n_sequences += std::count( buffer.begin(),
                                   buffer.begin() + ifile.gcount(), '\n' );
    }
    ifile.close();
    double end_time = omp_get_wtime();
    printf("(II) - %d lines in file\n", n_sequences);
    printf("(II) - It took %g seconds\n", end_time - start_time);
    return n_sequences;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//