#include <cstdio>
#include <vector>
#include <omp.h>
#include <getopt.h>
#include <sys/stat.h>

#include "../src/merger/in_file/merger_n_files.hpp"
#include "../src/tools/file_stats.hpp"

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int main(int argc, char *argv[])
{
    std::vector<std::string> i_files;
    std::string o_file;

    int verbose_flag = 0;
    int help_flag    = 0;

    static struct option long_options[] ={
            {"verbose",     no_argument, &verbose_flag, 1},
            {"help",        no_argument, 0, 'h'},
            {"input",       required_argument, 0,  'i'},
            {"output",      required_argument, 0,  'o'},
            {0, 0, 0, 0}
    };

    /* getopt_long stores the option index here. */
    int option_index = 0;
    int c;
    while( true )
    {
        c = getopt_long (argc, argv, "i:o:vh", long_options, &option_index);
        if (c == -1)
            break;

        switch ( c )
        {
            case 'i':
                i_files.push_back( optarg );
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
    if( i_files.size() < 2 )
    {
        printf ("(EE) There is not enought input files (%zu) !\n", i_files.size());
    }
    if( o_file.length() == 0 )
    {
        printf ("(EE) Output filename is undefined !\n");
    }

    if ( (optind < argc) || (help_flag == true) || (i_files.size() < 2) || (o_file.length() == 0) )
    {
        printf ("(II) Usage :\n");
        printf ("(II) ./merge_all -i <first file> -i <second file> -i <...> -o <output file>");
        printf ("(II)\n");
        printf ("(II) Options :\n");
        printf ("(II) - \n");
        putchar ('\n');
        exit( EXIT_FAILURE );
    }

    /*
     * Counting the number of SMER in the file (to allocate memory)
     */
    uint64_t start   = 0;

    printf("(II)\n");
    printf("(II) Document informations:\n");
    for(int i = 0; i < i_files.size(); i += 1)
    {
        file_stats fs( i_files[i] );
        printf("(II) - ");
        fs.printf_size();
        printf("\n");
        start += (fs.size_bytes / sizeof(uint64_t));
    }

    double start_time = omp_get_wtime();

    merge_n_files(i_files, o_file);

    double end_time = omp_get_wtime();

    file_stats fo( o_file );

    const uint64_t mizer_o = fo.size_bytes  / sizeof(uint64_t);
    const uint64_t skipped = start - mizer_o;

    printf("(II) Document (3) information\n");
    printf("(II) - filename      : %s\n", o_file.c_str());
    printf("(II) - filesize      : %llu bytes\n",  fo.size_bytes);
    if( fo.size_kb <= 66 )
        printf("(II) -               : %llu Kbytes\n", fo.size_kb);
    else
        printf("(II) -               : %llu Mbytes\n", fo.size_mb);
    printf("(II) - #minzer start : %llu elements\n", start);
    printf("(II) -       skipped : %llu elements\n", skipped);
    printf("(II) -         final : %llu elements\n", mizer_o);
    printf("(II)\n");
    printf("(II) - Exec. time    : %1.2f sec.\n", end_time - start_time);
    printf("(II)\n");

    return 0;
}