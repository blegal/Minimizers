#pragma once
#include <random>
#include <chrono>
#include <array>
#include <cstring>
#include <omp.h>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <sys/stat.h>
#include <cstdio>
#include <atomic>
#include <queue>
#include <filesystem>

#include "../crumsort/crumsort.hpp"

void external_sort (
    const std::string& infile,
    const std::string& outfile,
    const std::string& tmp_dir,
    const uint64_t n_colors,
    const uint64_t ram_value_MB,
    const bool keep_tmp_files,
    const bool verbose_flag,
    int n_threads
);

void external_sort_sparse (
    const std::string& infile,
    const std::string& outfile,
    const std::string& tmp_dir,
    const uint64_t n_colors,
    const uint64_t ram_value_MB,
    const bool keep_tmp_files,
    const bool verbose_flag,
    int n_threads
);

bool check_file_sorted(
    const std::string& filename,
    uint64_t n_uint_per_element,
    bool verbose_flag = true
);

bool check_file_sorted_sparse(
    const std::string& filename,
    uint64_t bits_per_color,
    bool verbose_flag = true
);