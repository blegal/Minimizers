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

int fast_atoi(const char *a)
{
    int res = (*a++) - '0';
    while( *a != '\0' )
    {
        res = res * 10 + (*a - 48);
        a  += 1;
    }
    return res;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
