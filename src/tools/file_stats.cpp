//
// Created by legal on 2/7/25.
//
#include "file_stats.hpp"
#include <sys/stat.h>
#include "colors.hpp"

file_stats::file_stats(const std::string filen)
{
    name = filen;

    struct stat file_status;
    if (stat(name.c_str(), &file_status) < 0) {
        error_section();
        printf("(EE) Error while evaluting the file (%s)\n", name.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    const uint64_t kilo = 1024;
    size_bytes = file_status.st_size;
    size_kb    = size_bytes / kilo;
    size_mb    = size_kb    / kilo;
}

void file_stats::printf_size() const
{
    if     ( size_kb < 10 ) printf("%14s [%5llu B ]   ", name.c_str(), size_bytes);
    else if( size_mb < 10 ) printf("%14s [%5llu KB]   ", name.c_str(), size_kb);
    else                    printf("%14s [%5llu MB]   ", name.c_str(), size_mb);
}
