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

    int verbose_flag = 0;
    int help_flag    = 0;

    static struct option long_options[] ={
            {"verbose",     no_argument, &verbose_flag, 1},
            {"help",         no_argument, 0, 'h'},
            {"input",        required_argument, 0, 'i'},
            {"output",       required_argument, 0,  'o'},
            {"n_colors",       required_argument, 0,  'c'},
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
        || (n_colors == 0) )
    {
        printf ("\n");
        printf ("Usage :\n");
        printf ("./extract_all_colors -i <string> -c <int> -o <string>\n");
        printf ("\n");
        printf ("Options :\n");
        printf (" -i (--file)   : The raw file to analyze\n");
        printf (" -c (--colors) : The number of colors in the file\n");
        printf (" -o (--output) : The filename template\n");
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

    //
    // On cree tous les fichiers
    //

    std::vector<FILE*> list_ou;
    for(int i = 0; i < n_colors; i += 1) {
        const std::string filen = o_file + "_" + std::to_string(i);
        FILE* fo = fopen( filen.c_str(), "w" );
        if( fo == NULL )
        {
            printf("(EE) An error corrured while openning the file (%s)\n", filen.c_str());
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit( EXIT_FAILURE );
        }
        list_ou.push_back( fo );
    }

    int count_e = 0;

    const int e_size = 1 + n_uint64_c;

    auto t1 = std::chrono::high_resolution_clock::now();

    int buff_elnt = 4096;
    int buff_u64  = buff_elnt * e_size;

    uint64_t* i_buff = new uint64_t[buff_u64];

    //
    // On cree tous les buffers vers les fichiers
    //

    std::vector<uint64_t*> list_o_buff;
    for(int i = 0; i < n_colors; i += 1) {
        uint64_t* o_buff = new uint64_t[buff_elnt];
        list_o_buff.push_back( o_buff );
    }

    //
    //
    //

    std::vector<int> list_cnt;
    for(int i = 0; i < n_colors; i += 1)
        list_cnt.push_back( 0 );

    //
    //
    //

    int e_reads;
    do{
        e_reads = fread(i_buff, sizeof(uint64_t), buff_u64, fi); // on lit P elements
        for(uint64_t j = 0; j < e_reads; j += e_size )
        {
            for(int c = 0; c < n_uint64_c; c += 1)
            {
                uint64_t value = i_buff[j + 1 + c];

                for(int x = 0; x < 64; x +=1)
                {
                    if( value & 0x01 ){
                        const int color = 64 * c + x;
                        uint64_t* liste = list_o_buff[color];
                        liste[ list_cnt[color] ] = i_buff[j];
                        list_cnt[color] += 1;
                    }
                    value = value >> 1;
                }
            }

            //
            // Doit'on flusher le buffer de sortie ?
            //
            for(int i = 0; i < n_colors; i += 1)
            {
                if( list_cnt[i] == buff_elnt )
                {
                    fwrite(list_o_buff[i], sizeof(uint64_t), list_cnt[i], list_ou[i]);
                    list_cnt[i] = 0;
                    count_e    += 1;
                }
            }
        }
    }while( e_reads == buff_u64 );

    //
    // On flush les données restantes
    //
    for(int i = 0; i < n_colors; i += 1)
        fwrite(list_o_buff[i], sizeof(uint64_t), list_cnt[i], list_ou[i]);

    printf("(II) # copied minimizers : %d\n", count_e);

    //
    // On détruit tous les buffers
    //
    for(int i = 0; i < n_colors; i += 1) {
        delete [] list_o_buff[i];
    }

    //
    // On ferme tous les ficiers
    //
    for(int i = 0; i < n_colors; i += 1) {
        fclose( list_ou[i] );
    }

    std::chrono::duration<double, std::milli> ms_double = (std::chrono::high_resolution_clock::now() - t1);
    double ms_time = ms_double.count();
    if( ms_time > 1000.0 )
        std::cout << "Elapsed time : " << (ms_time/1000.0) << "s\n";
    else
        std::cout << "Elapsed time : " << ms_time << "ms\n";

    return 0;
}