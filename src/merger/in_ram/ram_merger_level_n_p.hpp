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

extern void ram_merge_level_n_p(
        CFileBuffer* ifile_1,
        CFileBuffer* ifile_2,
        CFileBuffer* o_file,
        const int level_1,
        const int level_2);

