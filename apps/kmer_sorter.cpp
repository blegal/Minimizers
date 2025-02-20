#include <cstdio>
#include <algorithm>
#include <omp.h>
#include <getopt.h>

#include "../src/minimizer/extract_kmer_v3.hpp"
#include "../src/minimizer/extract_kmer_v4.hpp"

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

    int help_flag          = 0;
    int verbose_flag       = 0;
    int file_save_debug    = 0;
    int file_save_output   = 1;
    int kmer_size          = 27;

    int version            = 3;
    
    int buffer_in_ram      =   64; // On parle en MBytes
    int buffer_ou_ram      = 1024; // On parle en MBytes

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
                    {"kmer-size", required_argument, 0, 'k'},
                    {"v3",        no_argument, 0, '3'},
                    {"v4",        no_argument, 0, '4'},
                    {"buffer-in", required_argument, 0, 'I'},
                    {"buffer-ou",required_argument, 0, 'O'},
                    {"sorter",   required_argument, 0, 's'},
                    {"file",     required_argument, 0, 'i'},
                    {"output",   required_argument, 0, 'o'},
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

            case 'i':
                i_file = optarg;
                break;

            case 'o':
                o_file = optarg;
                break;

            case 's':
                algo = optarg;
                break;

            case 'I': // taille du buffer en MBytes
                buffer_in_ram = std::atoi( optarg );
                break;

            case 'O': // taille du buffer en MBytes
                buffer_ou_ram = std::atoi( optarg );
                break;

            case 'k':
                kmer_size = std::atoi( optarg );
                break;

            case '3':
                version = 3;
                break;

            case '4':
                version = 4;
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

    if( version == 3)
    {
        extract_kmer_v3(
                i_file,
                o_file,
                algo,
                kmer_size,
                buffer_ou_ram,
                file_save_output,
                verbose_flag,
                file_save_debug
        );
    }else if(version == 4){
        extract_kmer_v4(
                i_file,
                o_file,
                algo,
                kmer_size,
                buffer_in_ram,
                buffer_ou_ram,
                file_save_output,
                verbose_flag,
                file_save_debug
        );
    }

    return 0;
}
