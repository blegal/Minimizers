#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <omp.h>
#include <thread>
#include <execution>

extern void std_2cores(std::vector<uint64_t>& test);
