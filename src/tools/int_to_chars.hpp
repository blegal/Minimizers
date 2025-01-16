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

int int_to_char(int value, char* ptr)
{
    if( value >= 100 ){
        int unit = value%10;
        int diza = (value/10)%10;
        int cent = (value/100);
        ptr[0] = '0' + cent;
        ptr[1] = '0' + diza;
        ptr[2] = '0' + unit;
        return 3;

    }else if( value >= 10){
        int unit = value%10;
        int diza = (value/10)%10;
        ptr[0] = '0' + diza;
        ptr[1] = '0' + unit;
        return 2;

    }else{
        ptr[0] = '0' + value;
        return 1;
    }
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
