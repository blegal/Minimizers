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

extern bool SaveMiniToFileRAW(const std::string filename, const std::vector<uint64_t> list_hash);

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//