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

extern void minimizer_processing(
        const std::string& i_file,
        const std::string& o_file,
        const std::string& algo,
        const bool file_save_output,
        const bool worst_case_memory,
        const bool verbose_flag,
        const bool file_save_debug
    );
