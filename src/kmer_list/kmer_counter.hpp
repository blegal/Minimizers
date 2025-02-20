#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <omp.h>
#include <sstream>
#include <getopt.h>

extern void VectorDeduplication(std::vector<uint64_t>& values);
extern int  VectorDeduplication(std::vector<uint64_t>& values, const int n_elements);

extern void kmer_counter(std::vector<uint64_t>& values, std::vector<int>& counters);
extern int  kmer_counter(std::vector<uint64_t>& values, std::vector<int>& counters, const int n_elements);
