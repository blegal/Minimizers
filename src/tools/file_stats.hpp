//
// Created by legal on 2/7/25.
//
#ifndef COLOR_SORTER_FILE_STATS_HPP
#define COLOR_SORTER_FILE_STATS_HPP

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <fstream>



class file_stats {
public:

    file_stats(const std::string filen);

    void printf_size() const;

    uint64_t size_bytes;
    uint64_t size_kb;
    uint64_t size_mb;

    std::string name;
};


#endif //COLOR_SORTER_FILE_STATS_HPP
