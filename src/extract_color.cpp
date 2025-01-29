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


int main(int argc, char *argv[])
{
    //
    //
    //

    std::string i_file = "";
    std::string o_file = "";
    int n_colors = 0;
    int s_color  = 0;

    int verbose_flag = 0;
    int help_flag    = 0;

    static struct option long_options[] ={
            {"verbose",     no_argument, &verbose_flag, 1},
            {"help",         no_argument, 0, 'h'},
            {"input",        required_argument, 0, 'i'},
            {"output",       required_argument, 0,  'o'},
            {"n_colors",       required_argument, 0,  'c'},
            {"s_color",      required_argument, 0,  's'},
            {0, 0, 0, 0}
    };

    /* getopt_long stores the option index here. */
    int option_index = 0;
    int c;
    while( true )
    {
        c = getopt_long (argc, argv, "c:i:o:s:vh", long_options, &option_index);
        if (c == -1)
            break;

        switch ( c )
        {
            case 'c':
                n_colors = std::atoi( optarg );
                break;

            case 's':
                s_color  = std::atoi( optarg );
                break;

            case 'i':
                i_file = optarg;
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
    if ( (optind < argc) || (help_flag == true) || (i_file.size() == 0) || (o_file.size() == 0)
        || (n_colors == 0) || (s_color >= n_colors))
    {
        printf ("\n");
        printf ("Usage :\n");
        printf ("./extract_n_colors -i <string> -c <int> -o <string> -s <int>\n");
        printf ("\n");
        printf ("Options :\n");
        printf (" -i (--file)   : The raw file to analyze\n");
        printf (" -c (--colors) : The number of colors in the file\n");
        printf (" -o (--output) : The raw file to create\n");
        printf (" -s (--colors) : The color id to extract\n");
        printf ("\n");
        exit( EXIT_FAILURE );
    }

    const uint64_t size_bytes  = get_file_size(i_file);
    const uint64_t n_elements  = size_bytes / sizeof(uint64_t);

    ////////////////////////////////////////////////////////////////////////////////////

    uint64_t n_minimizr;
    uint64_t n_uint64_c;

    if( n_colors < 64  ){
        n_uint64_c  = 1;
        n_minimizr  = n_elements / (1 + n_uint64_c);
    }else{
        n_uint64_c  = (n_colors / 64);
        n_minimizr  = n_elements / (1 + n_uint64_c);
    }

    printf("(II) file size in bytes  : %llu\n", size_bytes);
    printf("(II) # uint64_t elements : %llu\n", n_elements);
    printf("(II) # minimizers        : %llu\n", n_minimizr);
    printf("(II) # of n_colors       : %d\n",   n_colors  );
    printf("(II) # uint64_t/n_colors : %llu\n", n_uint64_c);

    ////////////////////////////////////////////////////////////////////////////////////

    FILE* fi = fopen( i_file.c_str(), "r" );
    if( fi == NULL )
    {
        printf("(EE) An error corrured while openning the file (%s)\n", i_file.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    FILE* fo = fopen( o_file.c_str(), "w" );
    if( fo == NULL )
    {
        printf("(EE) An error corrured while openning the file (%s)\n", o_file.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    const uint64_t p_color = 1 + (s_color / 64); // pour sauter le minimizer
    const uint64_t i_color = 1ULL << (s_color%64);

    const int e_size = 1 + n_uint64_c;
    uint64_t buff[e_size];
    for(uint64_t i = 0; i < n_elements; i += e_size )
    {
        if( (i%(4*1024*1024)) == 0 )
            printf("%llu/%llu\n", i, n_elements);

        fread(buff, sizeof(uint64_t), e_size, fi);
        if( buff[p_color] & i_color )
        {
            fwrite(buff, sizeof(uint64_t), 1, fo);
        }
    }

    fclose(fi);
    fclose(fo);

    return 0;
}