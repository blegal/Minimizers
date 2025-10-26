#pragma once
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

extern void merge_level_n_p_final(
        const std::string& ifile_1,
        const std::string& ifile_2,
        const std::string& o_file,
        const std::string& o_file_sparse,
        const int level_1,
        const int level_2);

