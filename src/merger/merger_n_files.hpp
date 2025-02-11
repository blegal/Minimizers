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

extern void merge_n_files(
        const std::vector<std::string>& file_list,
        const std::string& o_file);
