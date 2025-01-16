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


int read_k_value(std::string filename)
{
    std::ifstream ifile( filename );
    if( ifile.is_open() == false )
    {
        printf("(EE) File does not exist !\n");
        return EXIT_FAILURE;
    }
    std::string line, seq;

    getline(ifile, line);

    ifile.close();

    std::stringstream ss(line);
    getline(ss, seq, '\t');

    int k_length = seq.length();
//   printf("(II) Sequence line = %s\n", seq.c_str());
//  printf("(II) k_length      = %d\n", k_length);

    return k_length;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
