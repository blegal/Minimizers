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

extern bool SaveRawToFile(const std::string filename, const std::vector<uint64_t> list_hash);
extern bool SaveRawToFile(const std::string filename, const std::vector<uint64_t> list_hash, const int n_elements);

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//