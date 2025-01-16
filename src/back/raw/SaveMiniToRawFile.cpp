#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <omp.h>
#include "../../progress/progressbar.h"

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

bool SaveMiniToFileRAW(const std::string filename, const std::vector<uint64_t> list_hash)
{
/*
    printf("(II)\n");
    printf("(II) Saving minimizer data set in [%s]\n", filename.c_str());
    progressbar *progress = progressbar_new("Saving minimizer values (RAW)",1);
    double start_time = omp_get_wtime();
*/
    std::string n_file = filename;
    FILE* f = fopen( n_file.c_str(), "w" );
    if( f == NULL )
    {
        printf("(EE) File does not exist !\n");
        exit( EXIT_FAILURE );
    }

    const int n_lines = list_hash.size();

    fwrite(list_hash.data(), sizeof(uint64_t), list_hash.size(), f);

    fclose( f );
/*
    double end_time = omp_get_wtime();

    progressbar_inc(progress);
    progressbar_finish(progress);

    printf("(II) - Execution time    = %f\n", end_time - start_time);
*/
    return true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//