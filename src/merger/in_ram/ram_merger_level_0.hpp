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

extern void ram_merge_level_0(
        const CFileBuffer* ifile_1,
        const CFileBuffer* ifile_2,
        const CFileBuffer* o_file);
