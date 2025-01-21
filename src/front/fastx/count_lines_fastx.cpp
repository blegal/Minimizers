#pragma once
#include "count_lines_fastx.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
uint64_t count_lines_fastx(std::string filename)
{
//    printf("(II) Counting the number of sequences (c)\n");
    FILE* f = fopen( filename.c_str(), "r" );
    if( f == NULL )
    {
        printf("(EE) File does not exist (%s))\n", filename.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
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