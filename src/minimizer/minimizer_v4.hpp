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

extern void minimizer_processing_v4(
        const std::string& i_file,
        const std::string& o_file,
        const std::string& algo,
        const int  ram_limit_in_MB,
        const bool file_save_output,
        const bool file_save_debug,
        const int k,
        const int m
    );

extern void minimizer_processing_stats_only(
        const std::string& i_file,
        std::vector<uint64_t>& results
    );
