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
        const uint64_t  ram_limit_in_MB,
        const bool file_save_output,
        const bool file_save_debug,
        const uint64_t k,
        const uint64_t m
    );
