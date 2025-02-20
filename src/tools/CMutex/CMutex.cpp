/**
  Copyright (c) 2012-2023 "Bordeaux INP, Bertrand LE GAL"
  bertrand.legal@ims-bordeaux.fr
  [http://legal.vvv.enseirb-matmeca.fr]

  This file is part of a LDPC library for realtime LDPC decoding
  on processor cores.
*/
#include "CMutex.hpp"
#include "../colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CMutex::CMutex()
{
    pthread_mutex_init(&mutex, NULL);
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CMutex::~CMutex()
{
    pthread_mutex_destroy(&mutex);
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void CMutex::lock()
{
    pthread_mutex_lock(&mutex);

}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void CMutex::unlock()
{
    pthread_mutex_unlock(&mutex);
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//