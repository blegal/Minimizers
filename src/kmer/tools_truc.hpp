#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <omp.h>
#include "nucl_encode.hpp"

#define MEM_UNIT 64ULL

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

uint64_t mask_right(uint64_t numbits){
    uint64_t mask = -(numbits >= MEM_UNIT) | ((1ULL << numbits) - 1ULL);
    return mask;
}

uint64_t mask_left(uint64_t numbits){
    return ~(mask_right(MEM_UNIT-numbits));
}

uint64_t encode(std::string kmer){
    uint64_t encoded = 0;
    for(int i = 0; i < (int)kmer.size(); i+=1)
    {
        encoded <<= 2;
        encoded |= nucl_encode(kmer[i]);
    }
    return encoded;
}

uint64_t encode(const char* kmer, const int kmer_size)
{
    uint64_t encoded = nucl_encode(kmer[0]);
    for(int i = 1; i < kmer_size; i+=1)
    {
        encoded <<= 2;
        encoded |= nucl_encode(kmer[i]);
    }
    return encoded;
}

std::string decode(uint64_t revhash, uint64_t k){
    std::string kmer;

    for (size_t i=0; i<k; i++){
        kmer = nucl_decode(revhash) + kmer;
        revhash >>= 2;
    }

    return kmer;
}


// Thomas Wang's integer hash functions. See <https://gist.github.com/lh3/59882d6b96166dfc3d8d> for a snapshot.
uint64_t bfc_hash_64(uint64_t key, uint64_t mask) {
    key = (~key + (key << 21)) & mask; // key = (key << 21) - key - 1;
    key = key ^ key >> 24;
    key = ((key + (key << 3)) + (key << 8)) & mask; // key * 265
    key = key ^ key >> 14;
    key = ((key + (key << 2)) + (key << 4)) & mask; // key * 21
    key = key ^ key >> 28;
    key = (key + (key << 31)) & mask;
    return key;
}


uint64_t bfc_hash_64_inv(uint64_t key, uint64_t mask){
    uint64_t tmp;

    // Invert key = key + (key << 31)
    tmp = (key - (key << 31));
    key = (key - (tmp << 31)) & mask;

    // Invert key = key ^ (key >> 28)
    tmp = key ^ key >> 28;
    key = key ^ tmp >> 28;

    // Invert key *= 21
    key = (key * 14933078535860113213ull) & mask;

    // Invert key = key ^ (key >> 14)
    tmp = key ^ key >> 14;
    tmp = key ^ tmp >> 14;
    tmp = key ^ tmp >> 14;
    key = key ^ tmp >> 14;

    // Invert key *= 265
    key = (key * 15244667743933553977ull) & mask;

    // Invert key = key ^ (key >> 24)
    tmp = key ^ key >> 24;
    key = key ^ tmp >> 24;

    // Invert key = (~key) + (key << 21)
    tmp = ~key;
    tmp = ~(key - (tmp << 21));
    tmp = ~(key - (tmp << 21));
    key = ~(key - (tmp << 21)) & mask;

    return key;
}


uint64_t kmer_to_hash(std::string kmer, uint64_t k){
    return bfc_hash_64(encode(kmer), mask_right(k*2));
}

std::string hash_to_kmer(uint64_t hash, uint64_t k){
    return decode(bfc_hash_64_inv(hash, mask_right(k*2)), k);
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
