#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <sstream>

class CMergeFile{
public:
    std::string name;
    int64_t     numb_colors;
    int64_t     real_colors;

    CMergeFile();
    CMergeFile(const std::string n, const int64_t nc, const int64_t rc);
    CMergeFile(const std::string n, const CMergeFile& cm1, const CMergeFile& cm2);
};
