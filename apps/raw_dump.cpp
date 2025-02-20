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

    std::string ifile;
    uint64_t n_colors = -1;
    int help_flag     = 0;

    static struct option long_options[] ={
            {"help",        no_argument, 0, 'h'},
            {"file",        required_argument, 0,  'f'},
            {"colors",      required_argument, 0,  'c'},
            {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;
    while( true )
    {
        c = getopt_long (argc, argv, "f:c:vh", long_options, &option_index);
        if (c == -1)
            break;

        switch ( c )
        {
            case 'f':
                ifile = optarg;
                break;

            case 'c':
                n_colors = std::atoi( optarg );
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

    if ( (optind < argc) || (help_flag == true) || (ifile.size() == 0) || (ifile.size() == 0) || (n_colors < 0) )
    {
        printf ("\n");
        printf ("Usage :\n");
        printf ("  ./raw_dump -f <string> -c <int>\n");
        printf ("\n");
        printf ("Options :\n");
        printf ("  -f (--file)   : The raw file to analyze\n");
        printf ("  -c (--colors) : The number of colors in the file\n");
        printf ("\n");
        exit( EXIT_FAILURE );
    }

    const uint64_t size_bytes  = get_file_size(ifile);
    const uint64_t n_elements  = size_bytes / sizeof(uint64_t);

    ////////////////////////////////////////////////////////////////////////////////////

    uint64_t n_minimizr;
    uint64_t n_uint64_c;
    if( n_colors == 0 ){
        n_minimizr  = n_elements;
        n_uint64_c  = 0;
    }else if( n_colors < 64  ){
        n_uint64_c  = 1;
        n_minimizr  = n_elements / (1 + n_uint64_c);
    }else{
        n_uint64_c  = ((n_colors + 63) / 64);
        n_minimizr  = n_elements / (1 + n_uint64_c);
    }

    const int scale = 1 + n_uint64_c;

    printf("(II) file size in bytes  : %llu\n", size_bytes);
    printf("(II) # uint64_t elements : %llu\n", n_elements);
    printf("(II) # minimizers        : %llu\n", n_minimizr);
    printf("(II) # of colors         : %llu\n", n_colors  );
    printf("(II) # uint64_t/colors   : %llu\n", n_uint64_c);

    ////////////////////////////////////////////////////////////////////////////////////

    std::vector<uint64_t> liste( n_elements );

    FILE* f = fopen( ifile.c_str(), "r" );
    if( f == NULL )
    {
        printf("(EE) An error corrured while openning the file (%s)\n", ifile.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    fread(liste.data(), sizeof(uint64_t), n_elements, f);
    fclose(f);

    if( n_colors == 0 ) {

        //
        // On se simplifie la vie car on n'a pas de couleurs a gerer
        //
        printf("-------+------------------+\n");
        printf("  idx  |     minimizers   |\n");
        printf("-------+------------------+\n");
        for(int i = 0; i < n_minimizr; i += 1) {
//          printf("%6d | %16.16llX |\n", i, liste[i]);
            const uint64_t miniz = liste[i];
            printf("%6d |\e[0;32m %16.16llX \e[0;37m|\n", i, miniz);
        }

    }else{

        //
        // On se simplifie la vie car on n'a pas de couleurs a gerer
        //

        for(int i = 0; i < n_minimizr; i += 1)
        {
            const int pos        = scale * i;
            const uint64_t miniz = liste[pos];

//          printf("%6d | %16.16llX | ", i, miniz);
            printf("%6d |\e[0;32m %16.16llX \e[0;37m| ", i, miniz);

            //
            // On affiche les couleurs en mode Hexa
            //

            for(int c = 0; c < n_uint64_c; c += 1)
            {
                const uint64_t value = liste[scale * i + 1 + c];
                printf("%16.16llX", value);
            }
            printf(" | ");

            //
            // On affiche les couleurs du minimizer
            //

            for(int c = 0; c < n_uint64_c; c += 1)
            {
                const uint64_t value = liste[scale * i + 1 + c];
                for(int x = 0; x < 64; x +=1)
                {
                    if( (value >> x) & 0x01 )
                        printf("%d ", 64 * c + x);
                }
            }
            printf("\n");
        }
    }

    return 0;
}
