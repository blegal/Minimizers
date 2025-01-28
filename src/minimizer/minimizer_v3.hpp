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

extern void minimizer_processing_v3(
        const std::string& i_file,
        const std::string& o_file,
        const std::string& algo,
        const int  ram_limit_in_MB,
        const bool file_save_output,
        const bool verbose_flag,
        const bool file_save_debug
    );
