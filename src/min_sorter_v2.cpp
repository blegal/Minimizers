#include <cstdio>
#include <algorithm>
#include <omp.h>
#include <getopt.h>

#include "minimizer/minimizer_v2.hpp"

#include "./minimizer/deduplication.hpp"
#include "front/count_file_lines.hpp"

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

    int verbose_flag      = 0;
    int file_save_loaded  = 0;
    int file_save_debug   = 0;
    int file_save_output  = 1;

    int worst_case_memory = 0;

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

    minimizer_processing_v2(
            i_file,
            o_file,
            algo,
            1024,
            file_save_output,
            verbose_flag,
            file_save_debug
    );

    return 0;
}

//
// AHX_ATRIOSF_7_1_C0URMACXX.IND4_clean.fastx => 164939720 lines
// (II) # k-mer            : 1979276616
// (II) # m-mer            : 25730596008
// (II) # minimizers       :  329879436
// (II) Number of skipped minizr :  891047976
// (II) Number of minimizers     : 1699435677
// (II) memory occupancy         :      12965 MB
// (II) - Number of samples (start) = 1699435677
// (II) - Number of samples (stop)  =  844741384
// (II) - Execution time    = 0h01m57s 0h02m33s

// (II) # k-mer            :  989638308
// (II) # minimizers       :  164939718
// (II) Number of ADN sequences  :   82469859
// (II) Number of k-mer          : 1277160061
// (II) Number of skipped minizr :  430079792
// (II) Number of minimizers     :  847080269
// (II) memory occupancy         :       6462 MB
// (II) Launching the simplification step
// (II) - Number of samples (start) =  847080269
// (II) - Number of samples (stop)  =  494471415
// (II) - Execution time    = 0h00m44s 0h00m55s
//(II) Launching the simplification step
//(II) - Number of samples (start) =  852355408
//(II) - Number of samples (stop)  =  483776049
//(II) - Execution time    = 1.686398
// 0h01m03s 0h00m16s
//(II) Document (3) information
//(II) - filename      : tara_2x.merge.raw
//(II) - filesize      : 6757931080 bytes
//(II) -               : 6444 Mbytes
//(II) - #minzer start : 978247464 elements
//(II) -       skipped : 133506079 elements
//(II) -         final : 844741385 elements
//(II) - 8.17


// 4x => 41234930 lines

// 32x => 5154367 lines