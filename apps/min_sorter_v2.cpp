#include <cstdio>
#include <algorithm>
#include <omp.h>
#include <getopt.h>

#include "../src/minimizer/minimizer_v2.hpp"

#include "../src/front/count_file_lines.hpp"

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

int main(int argc, char* argv[])
{

    std::string i_file = "./AHX_ACXIOSF_6_1_23_all.txt";
    std::string o_file = "./result";

    int help_flag         = 0;
    int verbose_flag      = 0;
    int file_save_debug   = 0;
    int file_save_output  = 1;

    int ram_alloc_start   = 1024; // Comme on parle en MB, cela fait 1 GB

    //
    //
    //

    std::string algo = "std::sort";

    static struct option long_options[] =
            {
                    /* These options set a flag. */
                    {"help",               no_argument, &help_flag,    1},
                    {"verbose",            no_argument, &verbose_flag,    1},
                    {"brief",              no_argument, &verbose_flag,    0},
                    {"save-debug",         no_argument, &file_save_debug, 1},
                    {"do-not-save-output", no_argument, &file_save_output, 0},
                    /* These options don’t set a flag.
                       We distinguish them by their indices. */
                    {"GB",           required_argument, 0, 'G'},
                    {"MB",           required_argument, 0, 'M'},
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
                i_file = optarg;
//              o_file = i_file + ".ordered";
                break;

                //
                //
                //
            case 'o':
                o_file = optarg;
                break;

                //
                // Sorting algorithm
                //
            case 's':
                algo = optarg;
                break;

            case 'M':
                ram_alloc_start = std::atoi( optarg );
                break;

            case 'G':
                ram_alloc_start = 1024 * std::atoi( optarg );
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
    if ( (optind < argc) || (help_flag == true) )
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
        putchar ('\n');
        printf ("Usage :\n");
        printf ("./min_sorter_v2 --file [filename] --output [filename] [options]");
        putchar ('\n');
        printf ("Options :\n");
        printf (" --file           (-i) [string] : the name of the input file that contains ADN sequence\n");
        printf (" --output         (-o) [string] : the name of the output file that containt minimizers at the end\n");
        printf (" --sorter         (-s) [string] : the name of the sorting algorithm to apply\n");
        printf("                        + std::sort       :\n");
        printf("                        + std_2cores      :\n");
        printf("                        + std_4cores      :\n");
        printf("                        + crumsort        : default\n");
        printf("                        + crumsort_2cores :\n");
        printf (" --MB             (-M) [int]    : initial memory allocation in MBytes\n");
        printf (" --GB             (-G) [int]    : initial memory allocation in GBytes\n");
        printf ("\n");
        printf ("Others :\n");
        printf (" --verbose        (-v)          : display debug informations\n");
        printf (" --save-debug                   : for debugging purpose\n");
        printf (" --do-not-save-output           : for debugging purpose\n");
        exit( EXIT_FAILURE );
    }

    minimizer_processing_v2(
            i_file,
            o_file,
            algo,
            ram_alloc_start, // increase by 50% when required
            file_save_output,
            verbose_flag,
            file_save_debug
    );

    return 0;
}
