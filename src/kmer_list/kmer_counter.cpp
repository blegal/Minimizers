#include "kmer_counter.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int  kmer_counter(std::vector<uint64_t>& values, std::vector<int>& counters, const int n_elements)
{
    uint64_t* ptr_i = values.data  () + 1;
    uint64_t* ptr_o = values.data  () + 1;
    int*      ptr_c = counters.data();
    //int64_t length  = values.size  ();

    uint64_t value = values[0];
    uint64_t count = 1;

    for(int64_t x = 1; x < n_elements; x += 1)
    {
        if( value != *ptr_i )
        {
            value   = *ptr_i; // on récupère la nouvelle valeur
            *ptr_o  = value;  // on memorise la nouvelle valeur
            *ptr_c  = count;  // on memorise la valeur du compteur (pour l'ancienne valeur)
            count   = 1;      // on reinitialise le compteur
            ptr_o  += 1;      // on avance d'une case le ptr de sortie
            ptr_c  += 1;      // on avance d'une case le ptr de compteur
        }else{
            count  += 1;
        }
        ptr_i += 1;
    }

    *ptr_c = count;

    int64_t new_length  = ptr_o - values.data();
    return new_length;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void kmer_counter(std::vector<uint64_t>& values, std::vector<int>& counters)
{
    uint64_t* ptr_i = values.data  () + 1;
    uint64_t* ptr_o = values.data  () + 1;
    int*      ptr_c = counters.data();
    int64_t length  = values.size  ();

    uint64_t value = values[0];
    uint64_t count = 1;

    for(int64_t x = 1; x < length; x += 1)
    {
        if( value != *ptr_i )
        {
            value   = *ptr_i; // on récupère la nouvelle valeur
            *ptr_o  = value;  // on memorise la nouvelle valeur
            *ptr_c  = count;  // on memorise la valeur du compteur (pour l'ancienne valeur)
            count   = 1;      // on reinitialise le compteur
            ptr_o  += 1;      // on avance d'une case le ptr de sortie
            ptr_c  += 1;      // on avance d'une case le ptr de compteur
        }else{
            count  += 1;
        }
        ptr_i += 1;
    }

    *ptr_c = count;

    int64_t new_length  = ptr_o - values.data();
    values.resize  ( new_length );
    counters.resize( new_length );
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//