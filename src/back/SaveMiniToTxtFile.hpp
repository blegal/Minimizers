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

extern bool SaveMiniToTxtFile   (const std::string filename, const std::vector<uint64_t> list_hash);
extern bool SaveMiniToTxtFile_v2(const std::string filename, const std::vector<uint64_t> list_hash);

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//