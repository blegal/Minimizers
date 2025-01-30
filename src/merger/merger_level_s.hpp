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

extern void merge_level_s(
        const std::string& i_file,
        const std::string& o_file);
