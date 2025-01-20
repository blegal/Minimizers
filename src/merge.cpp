#include <cstdio>
#include <fstream>
#include <vector>
#include <omp.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>

#include "./merger/merger_level_0.hpp"
#include "./merger/merger_level_1.hpp"
#include "./merger/merger_level_2_32.hpp"

#include "./merger/merger_in.hpp"

uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int main(int argc, char *argv[])
{
    std::string ifile_1;
    std::string ifile_2;
    std::string o_file;
    int f1_colors    = 0;
    int f2_colors    = 0;

    int verbose_flag = 0;
    int help_flag    = 0;

    static struct option long_options[] ={
            {"verbose",     no_argument, &verbose_flag, 1},
            {"help",        no_argument, 0, 'h'},
            {"first",       required_argument, 0,  'f'},
            {"second",      required_argument, 0,  's'},
            {"output",      required_argument, 0,  'o'},
            {"color",       required_argument, 0,  'c'},
            {"s-color",     required_argument, 0,  'C'},
            {0, 0, 0, 0}
    };

    /* getopt_long stores the option index here. */
    int option_index = 0;
    int c;
    while( true )
    {
        c = getopt_long (argc, argv, "f:s:c:C:o:vh", long_options, &option_index);
        if (c == -1)
            break;

        switch ( c )
        {
            case 'c':
                f1_colors = std::atoi( optarg );
                f2_colors = std::atoi( optarg );
                break;

            case 'C':
                f2_colors = std::atoi( optarg );
                break;

            case 'f':
                ifile_1 = optarg;
                break;

            case 's':
                ifile_2 = optarg;
                break;

            case 'o':
                o_file = optarg;
                break;

            case 'h':
                help_flag = true;
                break;

            case '?':
                break;

            default:
                abort ();
        }
    }

    /*
     * Print any remaining command line arguments (not options).
     * */
    if( ifile_1.size() == 0 )
        printf ("(EE) First file to merge is undefined !\n");
    if( ifile_2.size() == 0 )
        printf ("(EE) First file to merge is undefined !\n");
    if( f1_colors == 0 ) // c possible au debut
        printf ("(EE) The #colors of first file equals 0 !\n");
    if( f2_colors == 0 )
        printf ("(EE) The #colors of second file equals 0 !\n");
    if( f1_colors < f2_colors )
        printf ("(EE) The #colors of second file is higher than the first one !\n");

    if ( (optind < argc) || (help_flag == true) || (ifile_1.size() == 0) || (ifile_2.size() == 0)
                         || (f1_colors < 0)     || (f2_colors < 0)       || (f1_colors < f2_colors))
    {
        printf ("(II) Usage :\n");
        printf ("(II) ./merge -f <first file> -s <second file> -o <output file> -c <first color> -C <second color>");
        printf ("(II)\n");
        printf ("(II) Options :\n");
        printf ("(II) - \n");
        putchar ('\n');
        exit( EXIT_FAILURE );
    }

    /*
     * Counting the number of SMER in the file (to allocate memory)
     */

    const uint64_t size_1_bytes = get_file_size(ifile_1);
    const uint64_t size_2_bytes = get_file_size(ifile_2);

    const uint64_t size_1_Kbytes = size_1_bytes / 1024;
    const uint64_t size_2_Kbytes = size_2_bytes / 1024;

    const uint64_t size_1_Mbytes = size_1_Kbytes / 1024;
    const uint64_t size_2_Mbytes = size_2_Kbytes / 1024;

    const uint64_t n_uint64_f1   = (f1_colors == 0) ? 1 : (1 + ((f1_colors + 63)/64) );
    const uint64_t n_uint64_f2   = (f2_colors == 0) ? 1 : (1 + ((f2_colors + 63)/64) );

    const uint64_t mizer_1 = size_1_bytes / (n_uint64_f1 * sizeof(uint64_t));
    const uint64_t mizer_2 = size_2_bytes / (n_uint64_f2 * sizeof(uint64_t));

    printf("(II)\n");
    printf("(II) Document (1) information\n");
    printf("(II) - filename    : %s\n", ifile_1.c_str());
    printf("(II) - filesize    : %llu bytes\n", size_1_bytes);
    if( size_1_Kbytes <= 32 )
        printf("(II) -             : %llu Kbytes\n", size_1_Kbytes);
    else
        printf("(II) -             : %llu Mbytes\n", size_1_Mbytes);
    printf("(II) - #minimizers : %llu elements\n", mizer_1);
    printf("(II)\n");
    printf("(II) Document (2) information\n");
    printf("(II) - filename    : %s\n", ifile_2.c_str());
    printf("(II) - filesize    : %llu bytes\n", size_2_bytes);
    if( size_1_Kbytes <= 32 )
        printf("(II) -             : %llu Kbytes\n", size_2_Kbytes);
    else
        printf("(II) -             : %llu Mbytes\n", size_2_Mbytes);
    printf("(II) - #minimizers : %llu elements\n", mizer_2);
    printf("(II)\n");


    printf("(II) #colors f(1)  : %d\n", f1_colors);
    printf("(II) #colors f(2)  : %d\n", f2_colors);
    printf("(II) #colors ofile : %d\n", f1_colors + f2_colors);
    printf("(II)\n");

    double start_time = omp_get_wtime();

    merger_in(ifile_1, ifile_2, o_file, f1_colors, f2_colors);

    double end_time = omp_get_wtime();

    const uint64_t size_o_bytes  = get_file_size(o_file);
    const uint64_t size_o_Kbytes = size_o_bytes  / 1024;
    const uint64_t size_o_Mbytes = size_o_Kbytes / 1024;
    const uint64_t mizer_o       = size_o_bytes  / sizeof(uint64_t);

    const uint64_t start   = mizer_1 + mizer_2;
    const uint64_t skipped = start - mizer_o;

    printf("(II) Document (3) information\n");
    printf("(II) - filename      : %s\n", o_file.c_str());
    printf("(II) - filesize      : %llu bytes\n",  size_o_bytes);
    if( size_1_Kbytes <= 66 )
        printf("(II) -               : %llu Kbytes\n", size_o_Kbytes);
    else
        printf("(II) -               : %llu Mbytes\n", size_o_Mbytes);
    printf("(II) - #minzer start : %llu elements\n", start);
    printf("(II) -       skipped : %llu elements\n", skipped);
    printf("(II) -         final : %llu elements\n", mizer_o);
    printf("(II)\n");
    printf("(II) - Exec. time    : %1.2f sec.\n", end_time - start_time);
    printf("(II)\n");

    return 0;
}