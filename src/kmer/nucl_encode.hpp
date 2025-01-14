#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <omp.h>

#define MEM_UNIT 64ULL

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

uint64_t nucl_encode(const char nucl)
{
    return (nucl >> 1) & 0b11;
}

char nucl_decode(const uint64_t val)
{
    const uint64_t txt[] = {'A', 'C', 'T', 'G'};
    return txt[val & 0b11];
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
