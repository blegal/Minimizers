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

uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}


int main(int argc, char *argv[]) {

    if (argc < 2) {
        return EXIT_FAILURE;
    }

    std::string ifile = argv[1];

    int level = 0;
    if (argc >= 3) {
        level = std::atoi( argv[2] );
    }

    int scale = (level == 0 ? 1 : 2);

    const uint64_t size_bytes  = get_file_size(ifile);
    const uint64_t n_elements  = size_bytes / sizeof(uint64_t);
    const uint64_t n_minimizr  = n_elements / (level == 0 ? 1 : 2);

    std::vector<uint64_t> liste( n_elements );

    FILE* f = fopen( ifile.c_str(), "r" );
    fread(liste.data(), sizeof(uint64_t), n_elements, f);
    fclose(f);

    for(int i = 0; i < n_minimizr; i += 1)
    {
        printf("%6d | %16.16llX | ", i, liste[scale * i]);
        uint64_t value = liste[scale * i + 1];
        printf("%16.16llX | ", value);

        if( level != 0 )
        {
            for(int x = 0; x < 64; x +=1)
            {
                if( (value >> x) & 0x01 )
                    printf("%d ", (x+1));
            }
        }
        printf("\n");
    }

    return 0;
}