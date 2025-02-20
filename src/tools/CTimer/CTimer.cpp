/**
  Copyright (c) 2012-2023 "Bordeaux INP, Bertrand LE GAL"
  bertrand.legal@ims-bordeaux.fr
  [http://legal.vvv.enseirb-matmeca.fr]

  This file is part of a LDPC library for realtime LDPC decoding
  on processor cores.
*/
#include "CTimer.hpp"
#include "../colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CTimer::CTimer(bool _start){
    if(_start == true){
        t_start = std::chrono::steady_clock::now();
        isRunning = true;
    }else{
        isRunning = false;
    }
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CTimer::CTimer(){
    isRunning = false;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void CTimer::start(){
    if( isRunning == true ){
        warning_section();
        printf("(WW) CTimer :: trying to start a CTimer object that is already running !\n");
        printf("(WW) Warning location : %s %d\n", __FILE__, __LINE__);
        reset_section();
    }
    isRunning = true;
    t_start   = std::chrono::steady_clock::now();
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void CTimer::stop(){
    if( isRunning == false ){
        warning_section();
        printf("(WW) CTimer :: trying to stop a CTimer object that is not running !\n");
        printf("(WW) Warning location : %s %d\n", __FILE__, __LINE__);
        reset_section();
    }
    t_stop    = std::chrono::steady_clock::now();
    isRunning = false;
    last_run  = std::chrono::duration <double, std::micro> (t_stop - t_start).count();
    counter  += last_run;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void CTimer::reset(){
    t_start  = std::chrono::steady_clock::now();
    counter  = 0.0;
    last_run = 0.0;
 }
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
double CTimer::get_time_us(){
    if( isRunning == true ){
        t_stop    = std::chrono::steady_clock::now();
        last_run  = std::chrono::duration <double, std::micro> (t_stop - t_start).count();
    }
    return last_run;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
double CTimer::get_time_ms(){
    return get_time_us() / 1000.0;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
double CTimer::get_time_sec(){
    return get_time_ms() / 1000.0;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
double CTimer::get_time_sum_us(){
	if( isRunning == true ){
        printf("(EE) We should never be there. The timer was not correctly stopped !\n");
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
	}
    return counter;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
double CTimer::get_time_sum_ms(){
    return get_time_sum_us() / 1000.f;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
double CTimer::get_time_sum_sec(){
    return get_time_sum_ms() / 1000.f;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
