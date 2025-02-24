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

    if (argc < 3) {
        printf("(EE) Not enougth arguments...\n");
        exit( EXIT_FAILURE );
    }

    std::string ifile_1 = argv[1];
    std::string ifile_2 = argv[2];

    const uint64_t size_bytes_1  = get_file_size(ifile_1);
    const uint64_t size_bytes_2  = get_file_size(ifile_2);
    const uint64_t n_elements_1  = size_bytes_1 / sizeof(uint64_t);
    const uint64_t n_elements_2  = size_bytes_2 / sizeof(uint64_t);

    ////////////////////////////////////////////////////////////////////////////////////

    uint64_t n_minimizr_1  = n_elements_1;
    uint64_t n_minimizr_2  = n_elements_2;
    uint64_t n_minimizr    = (n_minimizr_1 < n_minimizr_2) ? n_minimizr_1 : n_minimizr_2;
    uint64_t n_uint64_c    = 0;
    uint64_t n_colors      = 0;

    printf("(II) file (1) size in bytes  : %llu\n", size_bytes_1);
    printf("(II) file (2) size in bytes  : %llu\n", size_bytes_2);
    printf("(II) # uint64_t elements (1) : %llu\n", n_elements_1);
    printf("(II) # uint64_t elements (2) : %llu\n", n_elements_2);
    printf("(II) # minimizers            : %llu\n", n_minimizr);
    printf("(II) # of colors             : %llu\n", n_colors  );
    printf("(II) # uint64_t/colors       : %llu\n", n_uint64_c);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    std::vector<uint64_t> liste_1( n_minimizr );

    FILE* f1 = fopen( ifile_1.c_str(), "r" );
    if( f1 == NULL )
    {
        printf("(EE) An error corrured while openning the file (%s)\n", ifile_1.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    fread(liste_1.data(), sizeof(uint64_t), n_minimizr, f1);
    fclose(f1);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    std::vector<uint64_t> liste_2( n_minimizr );

    FILE* f2 = fopen( ifile_2.c_str(), "r" );
    if( f2 == NULL )
    {
        printf("(EE) An error corrured while openning the file (%s)\n", ifile_2.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    fread(liste_2.data(), sizeof(uint64_t), n_minimizr, f2);
    fclose(f2);


    //
    // On se simplifie la vie car on n'a pas de couleurs a gerer
    //
    printf("-------+------------------+------------------+\n");
    printf("  idx  |  minimizers (1)  |  minimizers (2)  |\n");
    printf("-------+------------------+------------------+\n");
    for(int i = 0; i < n_minimizr; i += 1)
    {
        const uint64_t minz_1 = liste_1[i];
        const uint64_t minz_2 = liste_2[i];
        if( minz_1 !=  minz_2 )
        {
            for(int j = -16; j < 16; j += 1)
            {
                if( (i + j) <           0 ) continue;
                if( (i + j) >= n_minimizr ) continue;
                const uint64_t miniz_1 = liste_1[i+j];
                const uint64_t miniz_2 = liste_2[i+j];
                if( miniz_1 ==  miniz_2 ){
                    printf("%6d |\e[0;32m %16.16llX \e[0;37m|\e[0;32m %16.16llX \e[0;37m|\n", i+j, miniz_1, miniz_2);
                }else{
                    printf("%6d |\e[0;31m %16.16llX \e[0;37m|\e[0;31m %16.16llX \e[0;37m|\n", i+j, miniz_1, miniz_2);
                }
            }
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}