#include <string>
#include <iostream>
#include <vector>
#include <cstdint>
#include <sys/stat.h> 
#include <algorithm>

#include "../crumsort/crumsort.hpp"

void external_sort (
    const std::string& infile,
    const std::string& outfile,
    const std::string& tmp_dir,
    const uint64_t n_colors,
    const uint64_t ram_value_MB,
    const bool keep_tmp_files=false,
    const bool verbose_flag=false
);