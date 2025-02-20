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

#include "../../tools/CFileBuffer/CFileBuffer.hpp"

extern void ram_merger_in(
        const std::string& ifile_1,
        const std::string& ifile_2,
        const std::string& o_file,
        const int level_1,
        const int level_2);
