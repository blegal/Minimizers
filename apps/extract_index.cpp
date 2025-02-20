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
    std::string ofile;
    uint64_t n_colors = -1;
    int help_flag     = 0;

    static struct option long_options[] ={
            {"help",        no_argument, 0, 'h'},
            {"file",        required_argument, 0,  'f'},
            {"output",      required_argument, 0,  'o'},
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

            case 'o':
                ofile = optarg;
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

    if ( (optind < argc) || (help_flag == true) || (ifile.size() == 0) || (ofile.size() == 0) || (n_colors <= 0) )
    {
        printf ("\n");
        printf ("Usage :\n");
        printf ("  ./raw_dump -f <string> -c <int> -o <string>\n");
        printf ("\n");
        printf ("Options :\n");
        printf ("  -f (--file)   : The raw file to analyze\n");
        printf ("  -c (--colors) : The number of colors in the file (should be != 0)\n");
        printf ("  -o (--output) : The name of the generated index\n");
        printf ("\n");
        exit( EXIT_FAILURE );
    }

    const uint64_t size_bytes  = get_file_size(ifile);
    const uint64_t n_elements  = size_bytes / sizeof(uint64_t);

    ////////////////////////////////////////////////////////////////////////////////////

    uint64_t n_minimizr;
    int n_uint64_c;
    if( n_colors == 0 ){
        n_minimizr  = n_elements;
        n_uint64_c  = 0;
    }else if( n_colors < 64  ){
        n_uint64_c  = 1;
        n_minimizr  = (n_elements + 63) / (1 + n_uint64_c);
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

    auto t1 = std::chrono::high_resolution_clock::now();

    FILE* fi = fopen( ifile.c_str(), "r" );
    if( fi == NULL )
    {
        printf("(EE) An error occurred while opening the file (%s)\n", ifile.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    FILE* fo = fopen( ofile.c_str(), "w" );
    if( fo == NULL )
    {
        printf("(EE) An error occurred while opening the file (%s)\n", ofile.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    std::vector<uint64_t> buff_in(1024 * (1 + n_uint64_c));
    std::vector<uint32_t> buff_ou(1024 * (2*1 + 1 + n_colors));

    int cnt = 0;

    for(uint64_t v = 0; v < n_minimizr; v += 1024)
    {
        int n_bytes = fread(buff_in.data(), sizeof(uint64_t), buff_in.size(), fi);
        int nElements = n_bytes / (1 + n_uint64_c);

        for(int i = 0; i < nElements; i += 1)
        {
            const int pos        = scale * i;
            const uint64_t miniz = buff_in[pos];

            buff_ou[cnt++] = miniz         & 0xFFFFFFFF;
            buff_ou[cnt++] = (miniz >> 32) & 0xFFFFFFFF;

            std::vector<uint32_t> list_c;
            for(int c = 0; c < n_uint64_c; c += 1)
            {
                const uint64_t value = buff_in[scale * i + 1 + c];
                for(int x = 0; x < 64; x +=1)
                {
                    if( (value >> x) & 0x01 )
                        list_c.push_back( 64 * c + x );
                }
            }
            buff_ou[cnt++] = list_c.size();
            for(int n = 0; n < (int)list_c.size(); n += 1)
            {
                buff_ou[cnt++] = list_c[n];
            }
        }
        fwrite(buff_ou.data(), sizeof(uint32_t), cnt, fo);
    }

    fclose(fi);
    fclose(fo);

    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms_double = t2 - t1;
    double ms_time = ms_double.count();
    if( ms_time > 1000.0 )
        std::cout << "Elapsed time : " << (ms_time/1000.0) << "s\n";
    else
        std::cout << "Elapsed time : " << ms_time << "ms\n";

    return 0;
}
