/**
  Copyright (c) 2012-2023 "Bordeaux INP, Bertrand LE GAL"
  bertrand.legal@ims-bordeaux.fr
  [http://legal.vvv.enseirb-matmeca.fr]

  This file is part of a LDPC library for realtime LDPC decoding
  on processor cores.
*/

#ifndef _CTimer_
#define _CTimer_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <chrono>
using namespace std;

class CTimer
{
    
private:
    std::chrono::steady_clock::time_point t_start;
    std::chrono::steady_clock::time_point t_stop;

    bool isRunning = false;

    double counter  = 0.0; // la somme des périodes mesurées in u-seconds
    double last_run = 0.0; // le temps de la dernière période mesurée in u-seconds

public:
    CTimer(bool _start);
    CTimer();

    void start();
    void stop();
    void reset();

    double get_time_us();
    double get_time_ms();
    double get_time_sec();

    double get_time_sum_us();
    double get_time_sum_ms();
    double get_time_sum_sec();
};

#endif
