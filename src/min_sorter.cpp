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

#include "progress/progressbar.h"

#include "front/fast_fasta_file.hpp"

#include "sorting/std_2cores/std_2cores.hpp"
#include "sorting/std_4cores/std_4cores.hpp"
#include "sorting/crumsort_2cores/crumsort_2cores.hpp"

#include "./tools/read_k_value.hpp"
#include "front/count_lines.hpp"

#include "./tools/fast_atoi.hpp"

#include "./kmer/bfc_hash64.hpp"

#include "back/txt/SaveMiniToTxtFile.hpp"
#include "back/raw/SaveMiniToRawFile.hpp"

void my_sort(std::vector<uint64_t>& test)
{
    const int size = test.size();
    const int half = size / 2;

    std::vector<uint64_t> v1( half );
    std::vector<uint64_t> v2( half );

    vec_copy(v1, test.data(),        half );
    vec_copy(v2, test.data() + half, half );

    std::sort( v1.begin(), v1.end() );
    std::sort( v2.begin(), v2.end() );

    int ptr_1 = 0;
    int ptr_2 = 0;

    int i;
    for(i = 0; i < size; i += 1)
    {
        if( v1[ptr_1] < v2[ptr_2] )
        {
            test[i] = v1[ptr_1++];
        } else {
            test[i] = v2[ptr_2++];
        }
    }
    if( ptr_1 == half )
    {
        for(; i < size; i += 1)
            test[i] = v2[ptr_2++];
    }else{
        for(; i < size; i += 1)
            test[i] = v2[ptr_1++];
    }
    v1.clear();
    v2.clear();
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

std::chrono::steady_clock::time_point begin;

#include <omp.h>

void VectorDeduplication(std::vector<uint64_t>& values)
{
    printf("(II)\n");
    printf("(II) Launching the simplification step\n");
    printf("(II) - Number of samples (start) = %10ld\n", values.size());

    double start_time = omp_get_wtime();
    uint64_t* ptr_i = values.data() + 1;
    uint64_t* ptr_o = values.data() + 1;
    int64_t length  = values.size();

    uint64_t value = values[0];
    for(int64_t x = 1; x < length; x += 1)
    {
        if( value != *ptr_i )
        {
            value  = *ptr_i;
            *ptr_o = value;
            ptr_o += 1;
        }
        ptr_i += 1;
    }
    int64_t new_length  = ptr_o - values.data();
    values.resize( new_length );
    double end_time = omp_get_wtime();
    printf("(II) - Number of samples (stop)  = %10ld\n", values.size());
    printf("(II) - Execution time    = %f\n", end_time - start_time);
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

#define MEM_UNIT 64ULL

inline uint64_t mask_right(uint64_t numbits){
    uint64_t mask = -(numbits >= MEM_UNIT) | ((1ULL << numbits) - 1ULL);
    return mask;
}


bool vec_compare(std::vector<uint64_t> dst, std::vector<uint64_t> src)
{
    if(dst.size() != src.size())
        return false;

    const int length = src.size();
    for(int x = 0; x < length; x += 1)
        if( dst[x] != src[x] )
            return false;
    return true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

void minimizer_processing(
        const std::string& i_file    = "none",
        const std::string& o_file    = "none",
        const std::string& algo      = "crumsort",
        const bool file_save_output  = true,
        const bool worst_case_memory = true,
        const bool verbose_flag      = false,
        const bool file_save_debug   = false
        )
{

    /*
     * Counting the number of SMER in the file (to allocate memory)
     */
    const int n_lines = count_lines_c( i_file );    //

    /*
     * Reading the K value from the first file line
     */
    const int seq = read_k_value( i_file );  //
    const int kmer = 31;
    const int mmer = 19;
    const int z = kmer - mmer;

    //
    // Printing some information for the user
    //
    const uint64_t nb_kmer = (uint64_t)(seq  - kmer + 1) * (uint64_t)n_lines;
    const uint64_t nb_mmer = (uint64_t)(kmer - mmer + 1) * (uint64_t)nb_kmer;
    const uint64_t nb_mini = (uint64_t)nb_kmer / (uint64_t)(z / 2); // empirique

    const uint64_t max_bytes   = nb_kmer * sizeof(uint64_t);
    const uint64_t estim_bytes = nb_mini * sizeof(uint64_t);
    const uint64_t max_MB      = max_bytes   / 1024 / 1024;
    const uint64_t estim_MB    = estim_bytes / 1024 / 1024;

    if( verbose_flag == true )
    {
        printf("(II)\n");
        printf("(II) # of sequences : %d\n", n_lines);
        printf("(II) sequ. length       : %10d nuc.\n", seq);
        printf("(II) k-mer length       : %10d nuc.\n", kmer);
        printf("(II) m-mer length       : %10d nuc.\n", mmer);
        printf("(II) # m-mer/k-mer      : %10d\n", z);
        printf("(II)\n");
        printf("(II) # k-mer            : %10llu\n", nb_kmer);
        printf("(II) # m-mer            : %10llu\n", nb_mmer);
        printf("(II) # minimizers       : %10llu\n", nb_mini);
        printf("(II)\n");
        printf("(II) mem required (max) : %6llu MB\n", max_MB   );
        printf("(II) mem required (est) : %6llu MB\n", estim_MB );
        printf("(II)\n");
    }

    //
    // Creating buffers to speed up file parsing
    //

    char seq_value[4096]; bzero(seq_value, 4096);
    char kmer_b   [  48]; bzero(kmer_b,      48);
    char mmer_b   [  32]; bzero(mmer_b,      32);

    //
    // Allocating the object that performs fast file parsing
    //

    fast_fasta_file fasta_ifile(i_file);

    progressbar *progress;

    if( verbose_flag == true ) {
        progress = progressbar_new("Loading k-mers", 100);
    }
    const int prog_step = n_lines / 100;

    //
    // For all the lines in the file => load and convert
    //

    // Borne a calculer

    std::vector<uint64_t> liste_mini((estim_bytes / sizeof (uint64_t)) );
    if( worst_case_memory == true )
    {
        liste_mini.resize(max_bytes / sizeof (uint64_t) );
    }

    uint32_t kmer_cnt = 0;

    int n_minizer = 0;
    int n_skipper = 0;

    for(int l_number = 0; l_number < n_lines; l_number += 1)
    {
        bool not_oef = fasta_ifile.next_sequence(seq_value);
        if( not_oef == false )
            break;

        const int s_length = strlen( seq_value );

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // Buffer utilisé pour la fenetre glissante des hash des m-mer
        //
        uint64_t buffer[z+1]; // nombre de m-mer/k-mer
        for(int x = 0; x <= z; x += 1)
            buffer[x] = UINT64_MAX;

        uint64_t mask = mask_right(2 * mmer); // on masque

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // On initilise le calcul du premier m-mer
        //
        const char* ptr_kmer = seq_value;  // la position de depart du k-mer
        uint64_t current_mmer = 0;
        uint64_t cur_inv_mmer = 0;
        int cnt = 0;

        for(int x = 0; x < mmer - 1; x += 1)
        {
            //
            // On calcule les m-mers
            //
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer &= mask;

            cur_inv_mmer >>= 2;
            cur_inv_mmer |= (encoded << (2 * (mmer - 1)));

#if _DEBUG_CURR_
            if( verbose_flag )
                printf(" nuc [%c]: | %16.16llX |\n", ptr_kmer[cnt], current_mmer);
#endif
            cnt          += 1;
        }

        uint64_t minv = UINT64_MAX;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // On traite le premier k-mer
        //
        ////////////////////////////////////////////////////////////////////////////////////
#if _DEBUG_CURR_
        if( verbose_flag )
        {
            printf(" k: ");
            for(int x = 0; x < kmer; x += 1) printf("%c", ptr_kmer[x]);
            printf("\n");
        }
#endif
        ////////////////////////////////////////////////////////////////////////////////////

        for(int m_pos = 0; m_pos <= z; m_pos += 1)
        {
            //
            // On calcule les m-mers
            //
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer &= mask;

            cur_inv_mmer >>= 2;
            cur_inv_mmer |= (encoded << (2 * (mmer - 1)));

            const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;
//          const uint64_t canon  = canonical(current_mmer, 2 * 19);
            const uint64_t s_hash = bfc_hash_64(canon, mask);

//          printf(" nuc | %16.16llX | %16.16llX | <%16.16llX> | %16.16llX |\n", current_mmer, cur_inv_mmer, canon, cano2);

            buffer[m_pos]         = s_hash; // on memorise le hash du mmer
//            if( verbose_flag ) {
//                printf("   m: %s | %16.16llX | %16.16llX |\n", "", buffer[m_pos], buffer[m_pos]);
//            }
            minv                  = (s_hash < minv) ? s_hash : minv;
            cnt                  += 1; // on avance dans la sequence de nucleotides
        }

#if _DEBUG_CURR_
        if( verbose_flag )
        {
            printf("(+)-  min = | %16.16llX |\n", minv);
        }
#endif
        //
        // On pousse le premier minimiser dans la liste
        //
        if( n_minizer == 0 )
        {
            liste_mini[n_minizer++] = minv;
        }
        else if( liste_mini[n_minizer-1] != minv )
        {
            liste_mini[n_minizer++] = minv;
        }else{
            n_skipper += 1;
        }

        //
        //
        //

        kmer_cnt += 1;

        for(int k_pos = 1; k_pos <= s_length - kmer; k_pos += 1) // On traite le reste des k-mers
        {
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer &= mask;

            cur_inv_mmer >>= 2;
            cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); // cf Yoann

            const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;
            const uint64_t s_hash = bfc_hash_64(canon, mask);
            minv                  = (s_hash < minv) ? s_hash : minv;

            if( minv == buffer[0] )
            {
                minv = s_hash;
                for(int p = 0; p < z+1; p += 1) {
                    const uint64_t value = buffer[p+1];
                    minv = (minv < value) ? minv : value;
                    buffer[p] = value;
                }
                buffer[z] = s_hash; // on memorise le hash du dernier m-mer
            }else{
                for(int p = 0; p < z+1; p += 1) {
                    buffer[p] = buffer[p+1];
                }
                buffer[z] = s_hash; // on memorise le hash du dernier m-mer
            }

            ////////////////////////////////////////////////////////////////////////////////////
#if 0
            if( verbose_flag )
            {
                printf(" k: ");
                for(int x = 0; x < kmer; x += 1)
                    printf("%c", ptr_kmer[cnt - kmer + 1 + x]);
                printf("\n");
            }
#endif
            ////////////////////////////////////////////////////////////////////////////////////
#if 0
            if( verbose_flag ) {
                for (int p = 0; p < z + 1; p += 1) {
                    printf("   m: %s | %16.16llX | %16.16llX |\n", "", buffer[p], buffer[p]);
                }
            }
#endif

            if( liste_mini[n_minizer-1] != minv )
            {
                liste_mini[n_minizer++] = minv;
            }else{
                n_skipper += 1;
            }
/*
            if( liste_mini[liste_mini.size()-1] != minv )
            {
                liste_mini.push_back( minv );
#if 0
                if( verbose_flag )
                    printf("(+)   min = | %16.16llX |\n", miniv);
//                  printf("(+)   min = | %16.16llX (%16.16llX) |\n", miniv, current_mmer);
#endif
            }else{
#if 0
                if( verbose_flag )
                    printf("(-)   min = | %16.16llX |\n", miniv);
//                  printf("(-)   min = | %16.16llX (%16.16llX) |\n", miniv, current_mmer);
#endif
            }
*/
            kmer_cnt += 1;
            cnt      += 1;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        if( verbose_flag == true ) {
            if (l_number % prog_step == 0)
                progressbar_inc(progress);
        }
        /////////////////////////////////////////////////////////////////////////////////////
    }


    liste_mini.resize(n_minizer);

    /////////////////////////////////////////////////////////////////////////////////////
    if( verbose_flag == true ) {
        progressbar_finish(progress);
    }
    /////////////////////////////////////////////////////////////////////////////////////

    if( verbose_flag == true ) {
        printf("(II) Number of ADN sequences  : %10d\n", n_lines);
        printf("(II) Number of k-mer          : %10u\n", kmer_cnt);
        printf("(II) Number of skipped minizr : %10d\n", n_skipper);
        printf("(II) Number of minimizers     : %10zu\n", liste_mini.size());
        printf("(II) memory occupancy         : %10lu MB\n", (liste_mini.size() * sizeof(uint64_t)) / 1024 / 1024);
    }

    //
    // On stoque à l'indentique les données que l'on vient lire. Cette étape est uniquement utile
    // pour du debug
    //

    if( file_save_debug ){
        SaveMiniToTxtFile_v2(o_file + ".non-sorted-v2.txt", liste_mini);
    }

    if( verbose_flag == true ) {
        printf("(II)\n");
        printf("(II) Launching the sorting step\n");
        printf("(II) - Sorting algorithm = %s\n", algo.c_str());
        printf("(II) - Number of samples = %ld\n", liste_mini.size());
    }

    /////////////////////////////////////////////////////////////////////////////////////
    if( verbose_flag == true ) {
        progress = progressbar_new("Sorting the minimizer values", 1);
    }
    /////////////////////////////////////////////////////////////////////////////////////

    double start_time = omp_get_wtime();
    if( algo == "std::sort" ) {
        std::sort( liste_mini.begin(), liste_mini.end() );
    } else if( algo == "std_2cores" ) {
        std_2cores( liste_mini );
    } else if( algo == "std_4cores" ) {
        std_4cores( liste_mini );
    } else if( algo == "crumsort" ) {
        crumsort_prim( liste_mini.data(), liste_mini.size(), 9 /*uint64*/ );
    } else if( algo == "crumsort_2cores" ) {
        crumsort_2cores( liste_mini );
    } else {
        printf("(EE) Sorting algorithm is invalid  (%s)\n", algo.c_str());
        printf("(EE) Valid choices are the following:\n");
        printf("(EE) - std::sort       :\n");
        printf("(EE) - std_2cores      :\n");
        printf("(EE) - std_4cores      :\n");
        printf("(EE) - crumsort        :\n");
        printf("(EE) - crumsort_2cores :\n");
        exit( EXIT_FAILURE );
    }
    if( verbose_flag == true ) {
        double end_time = omp_get_wtime();
        printf("(II) - Execution time    = %f\n", end_time - start_time);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    if( verbose_flag == true ) {
        progressbar_inc   (progress);
        progressbar_finish(progress);
    }
    /////////////////////////////////////////////////////////////////////////////////////

    //
    // En regle général on save le résultat sauf lorsque l'on fait du benchmarking
    //

    if( file_save_debug )
    {
        SaveMiniToTxtFile_v2(o_file + ".sorted.txt", liste_mini);
        SaveMiniToFileRAW   (o_file + ".hash", liste_mini);
    }

    //
    // On stoque à l'indentique les données que l'on vient lire. Cette étape est uniquement utile
    // pour du debug
    //
    if( verbose_flag == true ) {
        printf("(II)\n");
        printf("(II) Vector deduplication step\n");
    }

    /////////////////////////////////////////////////////////////////////////////////////
    if( verbose_flag == true ) {
        progress = progressbar_new("Processing", 1);
    }
    /////////////////////////////////////////////////////////////////////////////////////

    VectorDeduplication( liste_mini );

    /////////////////////////////////////////////////////////////////////////////////////
    if( verbose_flag == true ) {
        progressbar_inc   (progress);
        progressbar_finish(progress);
    }
    /////////////////////////////////////////////////////////////////////////////////////

    if( file_save_debug ){
        SaveMiniToTxtFile_v2(o_file + ".txt", liste_mini);
    }

    if( file_save_output ){
        SaveMiniToFileRAW(o_file + ".raw", liste_mini);
    }
}

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

    int verbose_flag      = 0;
    int file_save_loaded  = 0;
    int file_save_debug   = 0;
    int file_save_output  = 1;

    int worst_case_memory = 0;

    //
    //
    //

    std::string algo = "std::sort";

    static struct option long_options[] =
            {
                    /* These options set a flag. */
                    {"verbose",           no_argument, &verbose_flag,    1},
                    {"brief",              no_argument, &verbose_flag,    0},
                    {"worst-case",         no_argument, &worst_case_memory,1},
                    {"save-loaded",        no_argument, &file_save_loaded,1},
                    {"save-debug",         no_argument, &file_save_debug, 1},
                    {"do-not-save-output", no_argument, &file_save_output, 0},
                    /* These options don’t set a flag.
                       We distinguish them by their indices. */
                    {"sorter",      required_argument, 0, 's'},
                    {"file",        required_argument, 0, 'i'},
                    {"output",      required_argument, 0, 'o'},
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

                //
                //
                //
            case 'i':
                printf("parsing -file option\n");
                i_file = optarg;
//              o_file = i_file + ".ordered";
                break;

                //
                //
                //
            case 'o':
                printf("parsing -output option\n");
                o_file = optarg;
                break;

                //
                // Sorting algorithm
                //
            case 's':
                algo = optarg;
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
    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
        putchar ('\n');
        printf ("Usage :\n");
        printf ("./kmer_sorter --file [kmer-file] [options]");
        putchar ('\n');
        printf ("Options :\n");
        printf (" --file [string]      : the name of the file that contains k-mers\n");
        printf (" --output             : the name of the file that sorted k-mers at the end\n");
        printf (" --verbose            : display debug informations\n");
        printf (" --save-loaded        : for debugging purpose\n");
        printf (" --save-debug         : for debugging purpose\n");
        printf (" --do-not-save-output : for debugging purpose\n");
        printf (" --sorter             : the name of the sorting algorithm to apply\n");
        printf (" --quotient           : the quotient size\n");
        exit( EXIT_FAILURE );
    }

#if 1
    minimizer_processing(
            i_file,
            o_file,
            algo,
            file_save_output,
            worst_case_memory,
            verbose_flag,
            file_save_debug
    );
#else
    /*
     * Counting the number of SMER in the file (to allocate memory)
     */
    const int n_lines = count_lines_c( i_file );    //

    /*
     * Reading the K value from the first file line
     */
    const int seq = read_k_value( i_file );  //
    const int kmer = 31;
    const int mmer = 19;
    const int z = kmer - mmer;

    //
    // Printing some information for the user
    //
    const uint64_t nb_kmer = (uint64_t)(seq  - kmer + 1) * (uint64_t)n_lines;
    const uint64_t nb_mmer = (uint64_t)(kmer - mmer + 1) * (uint64_t)nb_kmer;
    const uint64_t nb_mini = (uint64_t)nb_kmer / (uint64_t)(z / 2); // empirique

    const uint64_t max_bytes   = nb_kmer * sizeof(uint64_t);
    const uint64_t estim_bytes = nb_mini * sizeof(uint64_t);
    const uint64_t max_MB      = max_bytes   / 1024 / 1024;
    const uint64_t estim_MB    = estim_bytes / 1024 / 1024;

    printf("(II)\n");
    printf("(II) # of sequences : %d\n", n_lines);
    printf("(II) sequ. length       : %10d nuc.\n", seq);
    printf("(II) k-mer length       : %10d nuc.\n", kmer);
    printf("(II) m-mer length       : %10d nuc.\n", mmer);
    printf("(II) # m-mer/k-mer      : %10d\n", z);
    printf("(II)\n");
    printf("(II) # k-mer            : %10llu\n", nb_kmer);
    printf("(II) # m-mer            : %10llu\n", nb_mmer);
    printf("(II) # minimizers       : %10llu\n", nb_mini);
    printf("(II)\n");
    printf("(II) mem required (max) : %6llu MB\n", max_MB   );
    printf("(II) mem required (est) : %6llu MB\n", estim_MB );
    printf("(II)\n");

    //
    // Creating buffers to speed up file parsing
    //

    char seq_value[4096]; bzero(seq_value, 4096);
    char kmer_b   [  48]; bzero(kmer_b,      48);
    char mmer_b   [  32]; bzero(mmer_b,      32);

    //
    // Allocating the object that performs fast file parsing
    //

    fast_fasta_file fasta_ifile(i_file);

    progressbar *progress = progressbar_new("Loading k-mers",100);
    const int prog_step = n_lines / 100;

    //
    // For all the lines in the file => load and convert
    //

    // Borne a calculer

    std::vector<uint64_t> liste_mini((estim_bytes / sizeof (uint64_t)) );
    if( worst_case_memory == true )
    {
        liste_mini.resize(max_bytes / sizeof (uint64_t) );
    }

    uint32_t kmer_cnt = 0;

    int n_minizer = 0;
    int n_skipper = 0;

    for(int l_number = 0; l_number < n_lines; l_number += 1)
    {
        bool not_oef = fasta_ifile.next_sequence(seq_value);
        if( not_oef == false )
            break;

        const int s_length = strlen( seq_value );

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // Buffer utilisé pour la fenetre glissante des hash des m-mer
        //
        uint64_t buffer[z+1]; // nombre de m-mer/k-mer
        for(int x = 0; x <= z; x += 1)
            buffer[x] = UINT64_MAX;

        uint64_t mask = mask_right(2 * mmer); // on masque

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // On initilise le calcul du premier m-mer
        //
        const char* ptr_kmer = seq_value;  // la position de depart du k-mer
        uint64_t current_mmer = 0;
        uint64_t cur_inv_mmer = 0;
        int cnt = 0;

        for(int x = 0; x < mmer - 1; x += 1)
        {
            //
            // On calcule les m-mers
            //
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer &= mask;

            cur_inv_mmer >>= 2;
            cur_inv_mmer |= (encoded << (2 * (mmer - 1)));

            if( verbose_flag )
                printf(" nuc [%c]: | %16.16llX |\n", ptr_kmer[cnt], current_mmer);
            cnt          += 1;
        }

        uint64_t minv = UINT64_MAX;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // On traite le premier k-mer
        //
        ////////////////////////////////////////////////////////////////////////////////////
        if( verbose_flag )
        {
            printf(" k: ");
            for(int x = 0; x < kmer; x += 1) printf("%c", ptr_kmer[x]);
            printf("\n");
        }
        ////////////////////////////////////////////////////////////////////////////////////

        for(int m_pos = 0; m_pos <= z; m_pos += 1)
        {
            //
            // On calcule les m-mers
            //
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer &= mask;

            cur_inv_mmer >>= 2;
            cur_inv_mmer |= (encoded << (2 * (mmer - 1)));

            const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;
//          const uint64_t canon  = canonical(current_mmer, 2 * 19);
            const uint64_t s_hash = bfc_hash_64(canon, mask);

//          printf(" nuc | %16.16llX | %16.16llX | <%16.16llX> | %16.16llX |\n", current_mmer, cur_inv_mmer, canon, cano2);

            buffer[m_pos]         = s_hash; // on memorise le hash du mmer
//            if( verbose_flag ) {
//                printf("   m: %s | %16.16llX | %16.16llX |\n", "", buffer[m_pos], buffer[m_pos]);
//            }
            minv                  = (s_hash < minv) ? s_hash : minv;
            cnt                  += 1; // on avance dans la sequence de nucleotides
        }

        if( verbose_flag )
        {
            printf("(+)-  min = | %16.16llX |\n", minv);
        }

        //
        // On pousse le premier minimiser dans la liste
        //
        if( n_minizer == 0 )
        {
            liste_mini[n_minizer++] = minv;
        }
        else if( liste_mini[n_minizer-1] != minv )
        {
            liste_mini[n_minizer++] = minv;
        }else{
            n_skipper += 1;
        }
/*
        if( liste_mini.size() == 0 )
            liste_mini.push_back( minv );
        else if( liste_mini[liste_mini.size()-1] != minv )
            liste_mini.push_back( minv );
*/
        //
        //
        //

        kmer_cnt += 1;

        for(int k_pos = 1; k_pos <= s_length - kmer; k_pos += 1) // On traite le reste des k-mers
        {
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer &= mask;

            cur_inv_mmer >>= 2;
            cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); // cf Yoann

            const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;
            const uint64_t s_hash = bfc_hash_64(canon, mask);
            minv                  = (s_hash < minv) ? s_hash : minv;

            if( minv == buffer[0] )
            {
                minv = s_hash;
                for(int p = 0; p < z+1; p += 1) {
                    const uint64_t value = buffer[p+1];
                    minv = (minv < value) ? minv : value;
                    buffer[p] = value;
                }
                buffer[z] = s_hash; // on memorise le hash du dernier m-mer
            }else{
                for(int p = 0; p < z+1; p += 1) {
                    buffer[p] = buffer[p+1];
                }
                buffer[z] = s_hash; // on memorise le hash du dernier m-mer
            }

            ////////////////////////////////////////////////////////////////////////////////////
#if 0
            if( verbose_flag )
            {
                printf(" k: ");
                for(int x = 0; x < kmer; x += 1)
                    printf("%c", ptr_kmer[cnt - kmer + 1 + x]);
                printf("\n");
            }
#endif
            ////////////////////////////////////////////////////////////////////////////////////
#if 0
            if( verbose_flag ) {
                for (int p = 0; p < z + 1; p += 1) {
                    printf("   m: %s | %16.16llX | %16.16llX |\n", "", buffer[p], buffer[p]);
                }
            }
#endif

            if( liste_mini[n_minizer-1] != minv )
            {
                liste_mini[n_minizer++] = minv;
            }else{
                n_skipper += 1;
            }
/*
            if( liste_mini[liste_mini.size()-1] != minv )
            {
                liste_mini.push_back( minv );
#if 0
                if( verbose_flag )
                    printf("(+)   min = | %16.16llX |\n", miniv);
//                  printf("(+)   min = | %16.16llX (%16.16llX) |\n", miniv, current_mmer);
#endif
            }else{
#if 0
                if( verbose_flag )
                    printf("(-)   min = | %16.16llX |\n", miniv);
//                  printf("(-)   min = | %16.16llX (%16.16llX) |\n", miniv, current_mmer);
#endif
            }
*/
            kmer_cnt += 1;
            cnt      += 1;
        }

        if( l_number%prog_step == 0)
            progressbar_inc(progress);
    }

    liste_mini.resize(n_minizer);

    progressbar_finish(progress);

    printf("(II) Number of ADN sequences  : %10d\n",     n_lines);
    printf("(II) Number of k-mer          : %10u\n",     kmer_cnt);
//  printf("(II) Number of m-mer          : %10u\n",     mmer_cnt);
    printf("(II) Number of skipped minizr : %10d\n",     n_skipper);
    printf("(II) Number of minimizers     : %10zu\n",    liste_mini.size());
    printf("(II) memory occupancy         : %10lu MB\n", (liste_mini.size() * sizeof(uint64_t)) / 1024 / 1024 );

    //
    // On stoque à l'indentique les données que l'on vient lire. Cette étape est uniquement utile
    // pour du debug
    //
    if( file_save_debug )
    {
        SaveMiniToTxtFile_v2(o_file + ".non-sorted-v2.txt", liste_mini);
//      SaveMiniToTxtFile(o_file + ".non-sorted.txt", liste_mini);
    }

    printf("(II)\n");
    printf("(II) Launching the sorting step\n");
    printf("(II) - Sorting algorithm = %s\n",  algo.c_str());
    printf("(II) - Number of samples = %ld\n", liste_mini.size());

    progress = progressbar_new("Sorting the minimizer values",1);
    double start_time = omp_get_wtime();
    if( algo == "std::sort" ) {
        std::sort( liste_mini.begin(), liste_mini.end() );
    } else if( algo == "std_2cores" ) {
        std_2cores( liste_mini );
    } else if( algo == "std_4cores" ) {
        std_4cores( liste_mini );
    } else if( algo == "crumsort" ) {
        crumsort_prim( liste_mini.data(), liste_mini.size(), 9 /*uint64*/ );
    } else if( algo == "crumsort_2cores" ) {
        crumsort_2cores( liste_mini );
    } else {
        printf("(EE) Sorting algorithm is invalid  (%s)\n", algo.c_str());
        printf("(EE) Valid choices are the following:\n");
        printf("(EE) - std::sort       :\n");
        printf("(EE) - std_2cores      :\n");
        printf("(EE) - std_4cores      :\n");
        printf("(EE) - crumsort        :\n");
        printf("(EE) - crumsort_2cores :\n");
        exit( EXIT_FAILURE );
    }
    double end_time = omp_get_wtime();
    printf("(II) - Execution time    = %f\n", end_time - start_time);
    progressbar_inc(progress);
    progressbar_finish(progress);

    //
    // En regle général on save le résultat sauf lorsque l'on fait du benchmarking
    //

    if( file_save_debug )
    {
        SaveMiniToTxtFile_v2(o_file + ".sorted.txt", liste_mini);
        SaveMiniToFileRAW   (o_file + ".hash", liste_mini);
    }

    //
    // On stoque à l'indentique les données que l'on vient lire. Cette étape est uniquement utile
    // pour du debug
    //
    printf("(II)\n");
    printf("(II) Vector deduplication step\n");
    progress = progressbar_new("Processing",1);
    VectorDeduplication( liste_mini );
    progressbar_inc(progress);
    progressbar_finish(progress);
/*
    printf("(II)\n");
    printf("(II) Launching the simplification step\n");
    printf("(II) - Number of samples (start) = %ld\n", liste_mini.size());

    uint64_t* ptr_i = liste_mini.data() + 1;
    uint64_t* ptr_o = liste_mini.data() + 1;
    int64_t length  = liste_mini.size();

    uint64_t value = liste_mini[0];
    for(int64_t x = 1; x < length; x += 1)
    {
        if( value != *ptr_i )
        {
            value  = *ptr_i;
            *ptr_o = value;
            ptr_o += 1;
        }
        ptr_i += 1;
    }
    int64_t new_length  = ptr_o - liste_mini.data();
    liste_mini.resize( new_length );
    printf("(II) - Number of samples (stop)  = %ld\n", liste_mini.size());
*/
    if( file_save_debug )
    {
        SaveMiniToTxtFile_v2(o_file + ".txt", liste_mini);
    }

    if( file_save_output )
    {
        SaveMiniToFileRAW(o_file + ".raw", liste_mini);
    }
#endif
    return 0;
}

//
// AHX_ATRIOSF_7_1_C0URMACXX.IND4_clean.fastx => 164939720 lines
// (II) # k-mer            : 1979276616
// (II) # m-mer            : 25730596008
// (II) # minimizers       :  329879436
// (II) Number of skipped minizr :  891047976
// (II) Number of minimizers     : 1699435677
// (II) memory occupancy         :      12965 MB
// (II) - Number of samples (start) = 1699435677
// (II) - Number of samples (stop)  =  844741384
// (II) - Execution time    = 0h01m57s 0h02m33s

// (II) # k-mer            :  989638308
// (II) # minimizers       :  164939718
// (II) Number of ADN sequences  :   82469859
// (II) Number of k-mer          : 1277160061
// (II) Number of skipped minizr :  430079792
// (II) Number of minimizers     :  847080269
// (II) memory occupancy         :       6462 MB
// (II) Launching the simplification step
// (II) - Number of samples (start) =  847080269
// (II) - Number of samples (stop)  =  494471415
// (II) - Execution time    = 0h00m44s 0h00m55s
//(II) Launching the simplification step
//(II) - Number of samples (start) =  852355408
//(II) - Number of samples (stop)  =  483776049
//(II) - Execution time    = 1.686398
// 0h01m03s 0h00m16s
//(II) Document (3) information
//(II) - filename      : tara_2x.merge.raw
//(II) - filesize      : 6757931080 bytes
//(II) -               : 6444 Mbytes
//(II) - #minzer start : 978247464 elements
//(II) -       skipped : 133506079 elements
//(II) -         final : 844741385 elements
//(II) - 8.17


// 4x => 41234930 lines

// 32x => 5154367 lines