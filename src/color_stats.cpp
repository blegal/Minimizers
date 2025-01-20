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
    uint64_t n_colors = 0;
    int verbose_flag  = 0;
    int help_flag     = 0;

    static struct option long_options[] ={
            {"verbose",     no_argument, &verbose_flag, 1},
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

    const uint64_t size_bytes  = get_file_size(ifile);
    const uint64_t n_elements  = size_bytes / sizeof(uint64_t);

    ////////////////////////////////////////////////////////////////////////////////////

    uint64_t n_minimizr;
    uint64_t n_uint64_c;

    //
    // On verifie que le nombre de couleurs indiqué est non nul
    //

    if( ifile.size() == 0 ){
        printf("(EE) The input file is not set !\n");
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    if( n_colors == 0 ){
        printf("(EE) The number of color is set to ZERO !\n");
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    if ( (optind < argc) || (help_flag == true) || (ifile.size() == 0) || (n_colors < 0) )
    {
        printf ("(II) Usage :\n");
        printf ("(II) ./color_stats -f <input file> -c <number of color>");
        printf ("(II)\n");
        printf ("(II) Options :\n");
        printf ("(II) -f (--file)   : The raw file to analyze\n");
        printf ("(II) -c (--colors) : The number of colors in the file\n");
        putchar ('\n');
        exit( EXIT_FAILURE );
    }

    //
    // On estime les parametres qui vont être employés par la suite
    //

    if( n_colors < 64  ){
        n_uint64_c  = 1;
        n_minimizr  = n_elements / (1 + n_uint64_c);
    }else{
        n_uint64_c  = (n_colors / 64);
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

    //
    // On cree le vecteur qui va nous permettre de calculer l'histogramme
    //

    std::vector<uint64_t> histo( n_colors + 1 );
    for(int i = 0; i < histo.size(); i += 1)
    {
        histo[i] = 0;
    }

    //
    // On parcours l'ensemble des minimisers
    //
    auto t1 = std::chrono::high_resolution_clock::now();

    for(int i = 0; i < n_minimizr; i += 1)
    {
        const int pos        = scale * i;

        //
        // On parcours l'ensemble des couleurs du minimisers
        //

        int colors = 0;
        for(int c = 0; c < n_uint64_c; c += 1)
        {
            const uint64_t value = liste[pos + 1 + c];
            for(int x = 0; x < 64; x +=1)
            {
                if( (value >> x) & 0x01 )
                    colors += 1;
            }
        }
        histo[colors] += 1;
    }

    //
    // On va afficher l'ensemble des données issues de l'histo
    //

    printf("+------+------------+--------+---------+\n");
    printf("| Idx  | #Occurence | Occ. %% | Cumul.%% |\n");
    printf("+------+------------+--------+---------+\n");

    bool silent = false;
    int  last = -1;
    double sum = 0.0;
    for(uint64_t i = 1; i < histo.size(); i += 1)
    {
#if 0
        if( (histo[i] != last) )
        {
            double proba = 100.0 * (double)histo[i] / (double)n_minimizr;
            sum         += proba;
            printf("%6llu | %10llu | %6.3f | %6.3f |\n", i, histo[i], proba, sum);
            silent = false;
        } else if( silent == false )
        {
            printf("  .... |  ......... |  ..... |\n");
            silent = true;
        }
        last = histo[i];
#else
        if( histo[i] != 0 )
        {
            double proba = 100.0 * (double)histo[i] / (double)n_minimizr;
            sum         += proba;
            printf("%6llu | %10llu | %6.3f | %7.3f |\n", i, histo[i], proba, sum);
        }
#endif
    }
    printf("+------+------------+--------+---------+\n");

    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms_double = t2 - t1;
    double ms_time = ms_double.count();
    if( ms_time > 1000.0 )
        std::cout << "Elapsed time : " << (ms_time/1000.0) << "s\n";
    else
        std::cout << "Elapsed time : " << ms_time << "ms\n";

    return 0;
}