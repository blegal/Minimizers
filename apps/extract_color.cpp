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
        printf ("./extract_color -i <string> -c <int> -o <string> -s <int>\n");
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
        n_uint64_c  = ((n_colors + 63) / 64);
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

    int count_e = 0;

    const int e_size = 1 + n_uint64_c;

    auto t1 = std::chrono::high_resolution_clock::now();

/*
    printf("(II) p_color %d | p_color %d | p_color %d | \n", p_color, i_color, e_size);
*/
#if 1

    int buff_elnt = 4096;
    int buff_u64  = buff_elnt * e_size;

    uint64_t* i_buff = new uint64_t[buff_u64];
    uint64_t* o_buff = new uint64_t[buff_elnt];

    int cnt    = 0;
    int e_reads;
    do{
        e_reads = fread(i_buff, sizeof(uint64_t), buff_u64, fi); // on lit P elements
        for(uint64_t i = 0; i < e_reads; i += e_size ) {             // pour chacun des éléments

            //
            // Le minimiszer a t'il la bonne couleur ?
            //
            if (i_buff[i + p_color] & i_color) {
                o_buff[cnt++] = i_buff[i];
                count_e      += 1;
            }

            //
            // Doit'on flusher le buffer de sortie ?
            //
            if( cnt == buff_elnt ) {
                fwrite(o_buff, sizeof(uint64_t), cnt, fo);
                cnt = 0;
            }
        }
    }while( e_reads == buff_u64 );

    //
    // On flush les données restantes
    //
    fwrite(o_buff, sizeof(uint64_t), cnt, fo);

    delete [] i_buff;
    delete [] o_buff;


#else
    uint64_t buff[e_size];
    for(uint64_t i = 0; i < n_elements; i += e_size )
    {
        int nbytes = fread(buff, sizeof(uint64_t), e_size, fi);
        if( nbytes != e_size )
        {
            printf("(II) Oups !\n");
        }

        if( buff[p_color] & i_color )
        {
            fwrite(buff, sizeof(uint64_t), 1, fo);
            count_e += 1;
/*
            printf("%4d : ", i/e_size);
            for(uint64_t k = 0; k < e_size; k += 1 )
                printf("%16.16X ", buff[k]);
            printf("\n");
*/
        }
    }
#endif
    printf("(II) # copied minimizers : %d\n", count_e);

    std::chrono::duration<double, std::milli> ms_double = (std::chrono::high_resolution_clock::now() - t1);
    double ms_time = ms_double.count();
    if( ms_time > 1000.0 )
        std::cout << "Elapsed time : " << (ms_time/1000.0) << "s\n";
    else
        std::cout << "Elapsed time : " << ms_time << "ms\n";

    fclose(fi);
    fclose(fo);

    return 0;
}