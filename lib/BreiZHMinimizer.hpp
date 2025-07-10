// BreiZHMinimizer.hpp
#ifndef BREIZHMINIMIZER_HPP
#define BREIZHMINIMIZER_HPP

#include <cstdio>
#include <vector>
#include <omp.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <filesystem>

#include "../src/minimizer/minimizer_v2.hpp"
#include "../src/minimizer/minimizer_v3.hpp"
#include "../src/minimizer/minimizer_v4.hpp"
#include "../src/merger/in_file/merger_in.hpp"
#include "../src/merger/CMergeFile.hpp"

#include "../src/merger/in_file/merger_n_files.hpp"
#include "../src/merger/in_file/merger_n_files_lt64.hpp"
#include "../src/merger/in_file/merger_n_files_ge64.hpp"

#include "../src/tools/colors.hpp"
#include "../src/tools/CTimer/CTimer.hpp"
#include "../src/tools/file_stats.hpp"

void generate_minimizers(
    std::vector<std::string> filenames, 
    const std::string &output,
    const std::string &tmp_dir, 
    const int threads, 
    const int ram_value, 
    const int k, 
    const int m, 
    const uint64_t merge_step,
    const std::string &algo, 
    size_t verbose,
    bool skip_minimizer_step = false, 
    bool keep_minimizer_files = false,
    bool keep_merge_files = false
);

#endif