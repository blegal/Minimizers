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

#include "../src/front/fastx/read_fastx_file.hpp"
#include "../src/front/fastx_bz2/read_fastx_bz2_file.hpp"

#include "../src/sorting/std_2cores/std_2cores.hpp"
#include "../src/sorting/std_4cores/std_4cores.hpp"
#include "../src/sorting/crumsort_2cores/crumsort_2cores.hpp"

#include "../src/kmer_list/smer_deduplication.hpp"
#include "../src/front/read_k_value.hpp"
#include "../src/front/count_file_lines.hpp"

#include "../src/tools/fast_atoi.hpp"

#include "../src/back/txt/SaveMiniToTxtFile.hpp"
#include "../src/back/raw/SaveRawToFile.hpp"

#include "../src/hash/CustomMurmurHash3.hpp"

#include "../src/minimizer/minimizer.hpp"

#define _murmurhash_

void my_sort(std::vector<uint64_t>& test)
{
    const int size = test.size();
    const int half = size / 2;

    std::vector<uint64_t> v1( half );
    std::vector<uint64_t> v2( half );

    vec_copy(v1, test.data(),        half );
    vec_copy(v2, test.data() + half, half );

    std::sort( v1.begin(), v1.end() );
    std::sort( v2.begin(), v2.end() );

    int ptr_1 = 0;
    int ptr_2 = 0;

    int i;
    for(i = 0; i < size; i += 1)
    {
        if( v1[ptr_1] < v2[ptr_2] )
        {
            test[i] = v1[ptr_1++];
        } else {
            test[i] = v2[ptr_2++];
        }
    }
    if( ptr_1 == half )
    {
        for(; i < size; i += 1)
            test[i] = v2[ptr_2++];
    }else{
        for(; i < size; i += 1)
            test[i] = v2[ptr_1++];
    }
    v1.clear();
    v2.clear();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

std::chrono::steady_clock::time_point begin;

#include <omp.h>

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

#define MEM_UNIT 64ULL

inline uint64_t mask_right(uint64_t numbits){
    uint64_t mask = -(numbits >= MEM_UNIT) | ((1ULL << numbits) - 1ULL);
    return mask;
}


bool vec_compare(std::vector<uint64_t> dst, std::vector<uint64_t> src)
{
    if(dst.size() != src.size())
        return false;

    const int length = src.size();
    for(int x = 0; x < length; x += 1)
        if( dst[x] != src[x] )
            return false;
    return true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

int main(int argc, char* argv[])
{

    std::string i_file = "./AHX_ACXIOSF_6_1_23_all.txt";
    std::string o_file = "./result";

    int verbose_flag      = 0;
    int file_save_loaded  = 0;
    int file_save_debug   = 0;
    int file_save_output  = 1;

    int worst_case_memory = 1;

    //
    //
    //

    std::string algo = "std::sort";

    static struct option long_options[] =
            {
                    /* These options set a flag. */
                    {"verbose",           no_argument, &verbose_flag,    1},
                    {"brief",              no_argument, &verbose_flag,    0},
                    {"worst-case",         no_argument, &worst_case_memory,1},
                    {"save-loaded",        no_argument, &file_save_loaded,1},
                    {"save-debug",         no_argument, &file_save_debug, 1},
                    {"do-not-save-output", no_argument, &file_save_output, 0},
                    /* These options don’t set a flag.
                       We distinguish them by their indices. */
                    {"sorter",      required_argument, 0, 's'},
                    {"file",        required_argument, 0, 'i'},
                    {"output",      required_argument, 0, 'o'},
                    {0, 0, 0, 0}
            };

    /* getopt_long stores the option index here. */
    int option_index = 0;
    int c;
    while( true )
    {
        c = getopt_long (argc, argv, "s:q:i:o:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch ( c )
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;

                //
                //
                //
            case 'i':
                printf("parsing -file option\n");
                i_file = optarg;
//              o_file = i_file + ".ordered";
                break;

                //
                //
                //
            case 'o':
                printf("parsing -output option\n");
                o_file = optarg;
                break;

                //
                // Sorting algorithm
                //
            case 's':
                algo = optarg;
                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort ();
        }
    }

    /*
     * Print any remaining command line arguments (not options).
     * */
    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
        putchar ('\n');
        printf ("Usage :\n");
        printf ("./kmer_sorter --file [kmer-file] [options]");
        putchar ('\n');
        printf ("Options :\n");
        printf (" --file [string]      : the name of the file that contains k-mers\n");
        printf (" --output             : the name of the file that sorted k-mers at the end\n");
        printf (" --verbose            : display debug informations\n");
        printf (" --save-loaded        : for debugging purpose\n");
        printf (" --save-debug         : for debugging purpose\n");
        printf (" --do-not-save-output : for debugging purpose\n");
        printf (" --sorter             : the name of the sorting algorithm to apply\n");
        printf (" --quotient           : the quotient size\n");
        exit( EXIT_FAILURE );
    }


    minimizer_processing(
            i_file,
            o_file,
            algo,
            file_save_output,
            worst_case_memory,
            verbose_flag,
            file_save_debug
    );

    return 0;
}
