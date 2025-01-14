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

void fast_decode(uint64_t revhash, int k, char* ptr)
{
    char lut[4] = {'A', 'C', 'T', 'G'};
    for(int i = 0; i < k; i += 1)
    {
        ptr[ k-i-1 ] = lut[ revhash & 0x03 ];
        revhash >>= 2;
    }
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
