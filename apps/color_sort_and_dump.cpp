#include <cstdio>
#include <vector>
#include <algorithm>
#include <iostream>
#include <omp.h>
#include <getopt.h>
#include <sys/stat.h>

#include "../src/files/stream_reader_library.hpp"
#include "../src/tools/CTimer/CTimer.hpp"

uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
#if defined(__SSE4_2__)
#include <immintrin.h>
inline int popcount_u64_x86(const uint64_t _val)
{
    uint64_t val = _val;
	asm("popcnt %[val], %[val]" : [val] "+r" (val) : : "cc");
	return val;
}
#elif defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
inline uint64_t popcount_u64_arm(const uint64_t val)
{
    return vaddlv_u8(vcnt_u8(vcreate_u8((uint64_t) val)));
}
#endif
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
inline uint64_t popcount_u64_builtin(const uint64_t val)
{
    return __builtin_popcountll(val);
}
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
struct item{
    uint64_t  minimizer;
    uint16_t  n_colors;
    uint16_t* colors;
};
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
uint16_t* allocate_color_set(const std::vector<int>& vect)
{
    uint16_t* colors = (uint16_t*)malloc( vect.size() * sizeof(uint16_t) );
    for(int i = 0; i < vect.size(); i += 1)
        colors[i] = vect[i];
    return colors;
}

bool Sgreater_func (const item& e1, const item& e2)
{
    //
    // on tri par taille pour commencer
    //
    if ( e1.n_colors != e2.n_colors )
        return e1.n_colors > e2.n_colors;

    //
    // on tri par valeur
    //
    for(int i = 0; i < e1.n_colors; i += 1)
    {
        if( e1.colors[i] != e2.colors[i] )
        {
            return e1.colors[i] > e2.colors[i];
        }
    }
    return 0;
}

//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
int main(int argc, char *argv[]) {

    CTimer time_m(true);

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

    ////////////////////////////////////////////////////////////////////////////////////

    if ( (optind < argc) || (help_flag == true) || (ifile.size() == 0) || (n_colors < 0) )
    {
        printf ("\n");
        printf ("Usage :\n");
        printf ("  ./color_stats -f <string> -c <int>\n");
        printf ("\n");
        printf ("Options :\n");
        printf ("  -f (--file)   : The raw file to analyze\n");
        printf ("  -c (--colors) : The number of colors in the file\n");
        printf ("\n");
        exit( EXIT_FAILURE );
    }

    //
    // On estime les parametres qui vont être employés par la suite
    //

    uint64_t n_uint64_c;
    if( n_colors < 64  ){
        n_uint64_c  = 1;
    }else{
        n_uint64_c  = ((n_colors + 63) / 64);
    }

    const int eSize = 1 + n_uint64_c;

    ////////////////////////////////////////////////////////////////////////////////////

    const int n_buff_mini = ((64 * 1024 * 1024) / eSize); // On dimentionne la taille du buffer interne
    const int buffer_size = n_buff_mini * eSize;          // a grosso modo 64 MB de memoire

    //
    // Allocation du buffer en memoire
    //
    std::vector< uint64_t > buffer( buffer_size );

    //
    // Allocation de notre future structure de données
    //
    std::vector< item > liste;

    //
    // Allocation du tableau permettant de calculer l'histogramme
    //
    std::vector< uint64_t > histo(n_colors);
    for(int c = 0; c < n_colors; c += 1)
        histo[c] = 0;

    //
    // On ouvre un fichier "virtuel" pour acceder aux informations compressées ou pas
    //
    stream_reader* reader = stream_reader_library::allocate(ifile );
    if( reader->is_open() == false )
    {
        printf("(EE) An error corrured while openning the file (%s)\n", ifile.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    //
    // On parcours l'ensemble des minimisers
    //
    uint64_t cnt_elements = 0;
    uint64_t cnt_colors   = 0;
    uint64_t cnt_uint64   = 0;
    while( reader->is_eof() == false )
    {
        //
        // on precharge un sous ensemble de données
        //
        const int n_reads  = reader->read((char*)buffer.data(), sizeof(uint64_t), buffer.size());
        const int elements = n_reads / (1 + n_uint64_c);
        cnt_uint64 += n_reads;

        //
        // On parcours l'ensemble des minimizer que l'on a chargé
        //
        for(int m = 0; m < elements; m += 1) // le nombre total de minimizers
        {
            //
            // On compte le nombre de couleurs associé au minimizer
            //
            int n_bits = 0;

            //
            // On parcours toutes les couleurs du minimizer courant
            //
            std::vector<int> ll;
            for(int c = 0; c < n_uint64_c; c += 1)
            {
                const uint64_t value = buffer[eSize * m + 1 + c];

                if( value == 0 )
                    continue;

                for(int x = 0; x < 64; x +=1)
                {
                    if( (value >> x) & 0x01 )
                    {
                        ll.push_back(64 * c + x);
                        cnt_colors += 1;
                        n_bits     += 1;
                    }
                }
            }
            histo[n_bits] += 1;

            item ii;
            ii.minimizer = buffer[eSize * m + 1]; // la valeur du minimizer
            ii.n_colors  = ll.size();
            ii.colors    = allocate_color_set( ll );
            liste.push_back( ii );
            cnt_elements += 1;

            if( cnt_elements%1000000 == 0 ){
                const int64_t mem_bytes  = (cnt_elements * sizeof(item)) + sizeof(uint16_t) * cnt_colors;
                const int64_t mem_kbytes = mem_bytes  / 1024;
                const int64_t mem_mbytes = mem_kbytes / 1024;
                printf("%16lld elements | %16lld bytes | %16lld kbytes | %16lld mbytes | %6.3f%%\n", cnt_elements, mem_bytes, mem_kbytes, mem_mbytes, 100.0 * (double)cnt_elements/(double)933491795);
            }

        }

    }

    //
    //
    //

    reader->close();
    delete reader;

    //
    // On va afficher l'ensemble des données issues de l'histo
    //
    const float rate = (float)cnt_colors / (float)cnt_elements;
    const uint64_t file_size_bytes  = cnt_uint64 * sizeof(uint64_t);
    const uint64_t file_size_kbytes = file_size_bytes  / 1024;
    const uint64_t file_size_mbytes = file_size_kbytes / 1024;

    const uint64_t mem_size_bytes   = cnt_elements * (sizeof(uint64_t) + sizeof(uint64_t*) + sizeof(uint64_t)) + (cnt_colors * sizeof(uint16_t));
    const uint64_t mem_size_kbytes  = mem_size_bytes  / 1024;
    const uint64_t mem_size_mbytes  = mem_size_kbytes / 1024;

    printf("(II) file size in  bytes  : %llu\n",  file_size_bytes);
    printf("(II)              Kbytes  : %llu\n",  file_size_kbytes);
    printf("(II)              Mbytes  : %llu\n",  file_size_mbytes);
    printf("(II) # uint64_t elements  : %llu\n",  cnt_uint64  );
    printf("(II) # minimizers         : %llu\n",  cnt_elements);
    printf("(II) # of colors          : %llu\n",  cnt_colors  );
    printf("(II) # uint64_t/colors    : %1.3f\n", rate);
    printf("(II) RAM size in  bytes   : %llu\n",  mem_size_bytes);
    printf("(II)             Kbytes   : %llu\n",  mem_size_kbytes);
    printf("(II)             Mbytes   : %llu\n",  mem_size_mbytes);

    //
    // On va afficher l'ensemble des données issues de l'histo
    //
    printf("+------+------------+--------+---------+\n");
    printf("| Idx  | #Occurence | Occ. %% | Cumul.%% |\n");
    printf("+------+------------+--------+---------+\n");
    double sum = 0.0;
    for(uint64_t i = 1; i < histo.size(); i += 1)
    {
        if( histo[i] != 0 )
        {
            double proba = 100.0 * (double)histo[i] / (double)cnt_elements;
            sum         += proba;
            printf("%6llu | %10llu | %6.3f | %7.3f |\n", i, histo[i], proba, sum);
        }
    }
    printf("+------+------------+--------+---------+\n");

    double ms_time = time_m.get_time_ms();
    if( ms_time > 1000.0 )
        std::cout << "Elapsed time : " << (ms_time/1000.0) << "s\n";
    else
        std::cout << "Elapsed time : " << ms_time << "ms\n";

    std::sort( liste.begin(), liste.end(), &Sgreater_func);

    for(uint64_t i = 1; i < liste.size(); i += 1)
    {
        const item ii = liste[i];
        printf("%6d |\e[0;32m %16.16llX \e[0;37m| [3d]", i, ii.minimizer, ii.n_colors);

        for(int c = 0; c < ii.n_colors; c += 1)
            printf("%d ", ii.colors[c]);

        printf("\n");
    }

    ms_time = time_m.get_time_ms();
    if( ms_time > 1000.0 )
        std::cout << "Elapsed time : " << (ms_time/1000.0) << "s\n";
    else
        std::cout << "Elapsed time : " << ms_time << "ms\n";

    return 0;
}