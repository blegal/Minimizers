/**
  Copyright (c) 2012-2023 "Bordeaux INP, Bertrand LE GAL"
  bertrand.legal@ims-bordeaux.fr
  [http://legal.vvv.enseirb-matmeca.fr]

  This file is part of a LDPC library for realtime LDPC decoding
  on processor cores.
*/

#ifndef _CMutex_
#define _CMutex_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <chrono>
using namespace std;

class CMutex
{
private:
    pthread_mutex_t mutex;

public:
     CMutex();
    ~CMutex();

    void lock();
    void unlock();
};

#endif
