// BreiZHMinimizer.hpp
#ifndef BREIZHMINIMIZER_HPP
#define BREIZHMINIMIZER_HPP

#include <vector>
#include <string>
#include <cstdio>
#include <fstream>
#include <omp.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <filesystem>

#include "../src/minimizer/minimizer_v4.hpp"
#include "../src/merger/merger_in.hpp"

#include "../src/tools/colors.hpp"
#include "../src/tools/CTimer.hpp"
#include "../src/tools/file_stats.hpp"

void generate_minimizers(std::vector<std::string> filenames, const std::string &tmp_dir, const int threads, const int ram_value, const int k, const int m, size_t verbose);

#endif