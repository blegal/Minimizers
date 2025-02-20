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
#include <sys/stat.h>
#include <dirent.h>
#include <vector>

extern void merge_n_files_greater_than_64_colors(
        const std::vector<std::string>& file_list,
        const int64_t n_in_colors,
        const std::string& o_file);
