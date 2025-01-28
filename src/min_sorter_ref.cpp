#include <cstdio>
#include <vector>
#include <algorithm>
#include <omp.h>
#include <getopt.h>

#include "progress/progressbar.h"
#include "hash/CustomMurmurHash3.hpp"

#include "front/fastx/read_fastx_file.hpp"

#include "sorting/std_2cores/std_2cores.hpp"
#include "sorting/std_4cores/std_4cores.hpp"
#include "sorting/crumsort_2cores/crumsort_2cores.hpp"

#include "front/read_k_value.hpp"
#include "front/count_file_lines.hpp"

#include "back/txt/SaveMiniToTxtFile.hpp"
#include "back/raw/SaveRawToFile.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
std::chrono::steady_clock::time_point begin;
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
#define _murmurhash_

void VectorDeduplication(std::vector<uint64_t>& values)
{
    printf("(II)\n");
    printf("(II) Launching the simplification step\n");
    printf("(II) - Number of samples (start) = %ld\n", values.size());

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
    printf("(II) - Number of samples (stop)  = %ld\n", values.size());
    printf("(II) - Execution time    = %f\n", end_time - start_time);
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
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

int main(int argc, char* argv[])
{

    std::string i_file = "./AHX_ACXIOSF_6_1_23_all.txt";
    std::string o_file = "./result";

    int verbose_flag     = 0;
    int file_save_loaded = 0;
    int file_save_debug  = 0;
    int file_save_output = 1;


    //
    //
    //

    std::string algo = "std::sort";

    static struct option long_options[] =
            {
                    /* These options set a flag. */
                    {"verbose",           no_argument, &verbose_flag,    1},
                    {"brief",              no_argument, &verbose_flag,    0},
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

    /*
     * Counting the number of SMER in the file (to allocate memory)
     */
    const int n_lines = count_file_lines( i_file );    //

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
    const uint64_t nb_mini = (uint64_t)nb_kmer / (uint64_t)(z);

    const uint64_t MB = nb_mini * sizeof(uint64_t) / 1024 / 1024;

    printf("(II)\n");
    printf("(II) # of sequences : %d\n", n_lines);
    printf("(II) sequ. length   : %d\n", seq);
    printf("(II) k-mer length   : %d\n", kmer);
    printf("(II) m-mer length   : %d\n", mmer);
    printf("(II) # m-mer/k-mer  : %d\n", z);
    printf("(II)\n");
    printf("(II) # k-mer        : %llu\n", nb_kmer);
    printf("(II) # m-mer        : %llu\n", nb_mmer);
    printf("(II) # minimizers   : %llu\n", nb_mini);
    printf("(II)\n");
    printf("(II) memory occupancy = %6llu MB\n", MB );
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

    read_fastx_file fasta_ifile(i_file);

    progressbar *progress = progressbar_new("Loading k-mers",100);
    const int prog_step = n_lines / 100;

    //
    // For all the lines in the file => load and convert
    //

    // Borne a calculer

    std::vector<uint64_t> liste_mini( 2 * MB);

    uint32_t kmer_cnt = 0;
    uint32_t mmer_cnt = 0;

    for(int l_number = 0; l_number < n_lines; l_number += 1)
    {
        bool not_oef = fasta_ifile.next_sequence(seq_value);
        if( not_oef == false )
            break;

        const int s_length = strlen( seq_value );
        if( verbose_flag )
            printf("%s\n", seq_value);

        //
        // Pour tous les kmers d'une sequence
        //
        for(int k_pos = 0; k_pos < s_length - kmer + 1; k_pos += 1) // attention au <=
        {
            memcpy(kmer_b, seq_value + k_pos, kmer);

            uint64_t minv = UINT64_MAX;

            if( verbose_flag )
            {
                printf(" k-mer (%2d) : %s\n", k_pos, kmer_b);
            }

            //
            // On calcule le début du m-mer
            //
            printf("       m-mer                            | curr-m-mer       | curr-m-mer-inv   | cannonic-hash    |> minimum\n");
            for(int m_pos = 0; m_pos < kmer - mmer + 1; m_pos += 1) // attention au <=
            {
                memcpy(mmer_b, kmer_b + m_pos, mmer);

                uint64_t current_mmer = 0;
                uint64_t cur_inv_mmer = 0;
                for(int x = 0; x < mmer; x += 1)
                {
                    const uint64_t encoded = ((mmer_b[x] >> 1) & 0b11); // conversion ASCII => 2bits (Yoann)
                    current_mmer <<= 2;
                    current_mmer |=  encoded;
                    cur_inv_mmer >>= 2;
//                  cur_inv_mmer |= (encoded << (2 * (mmer - 1)));
                    cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); // cf Yoann
                }

                const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;

                // On hash le minimizer
                uint64_t tab[2];
                CustomMurmurHash3_x64_128<8> ( &canon, 42, tab );
                const uint64_t s_hash = tab[0];

                minv = (s_hash < minv) ? s_hash : minv;
                mmer_cnt += 1;

                if( verbose_flag )
                {
                    printf("       m-mer (%2d) : %s | %16.16llX | %16.16llX | %16.16llX |> %16.16llX\n", m_pos, mmer_b, current_mmer, cur_inv_mmer, s_hash, minv);
                }
            }

            if( verbose_flag )
            {
                printf("        mini : | %16.16llX\n", minv);
            }

            if( liste_mini.size() == 0 )
            {
                liste_mini.push_back( minv );
                if( verbose_flag )
                    printf("(+)-      min = | %16.16llX |\n", minv);
            }
            else if( liste_mini[liste_mini.size()-1] != minv )
            {
                liste_mini.push_back( minv );
                if( verbose_flag )
                {
                    if( k_pos == 0 )
                        printf("(+)-  min = | %16.16llX |\n", minv);
                    else
                        printf("(+)   min = | %16.16llX |\n", minv);
                }
            }
            else
            {
                if( verbose_flag )
                    printf("(-)   min = | %16.16llX |\n", minv);
            }
            kmer_cnt += 1;

        } // fin de boucle sur les k-mers

        if( l_number%prog_step == 0)
            progressbar_inc(progress);
    }

    progressbar_finish(progress);

    printf("(II) Number of ADN sequences  : %d\n",     n_lines);
    printf("(II) Number of k-mer          : %u\n",     kmer_cnt);
    printf("(II) Number of m-mer          : %u\n",     mmer_cnt);
    printf("(II) Number of minimizers     : %zu\n",    liste_mini.size());
    printf("(II) memory occupancy         : %6lu MB\n", liste_mini.size() * sizeof(uint64_t) / 1024 / 1024 );

    //
    // On stoque à l'indentique les données que l'on vient lire. Cette étape est uniquement utile
    // pour du debug
    //
    if( file_save_output )
        SaveMiniToTxtFile(o_file + ".ref.non-sorted.txt", liste_mini);

    printf("(II)\n");
    printf("(II) Launching the sorting step\n");
    printf("(II) - Sorting algorithm = %s\n",  algo.c_str());
    printf("(II) - Number of samples = %ld\n", liste_mini.size());

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

    //
    // En regle général on save le résultat sauf lorsque l'on fait du benchmarking
    //

    if( file_save_output )
        SaveMiniToTxtFile(o_file + ".ref.sorted.txt", liste_mini);

    //
    // On stoque à l'indentique les données que l'on vient lire. Cette étape est uniquement utile
    // pour du debug
    //
    if( file_save_debug )
        SaveRawToFile(o_file + ".hash", liste_mini);

    /////////////////////////////////////////////////////////////////////////////////////

    VectorDeduplication( liste_mini );

    /////////////////////////////////////////////////////////////////////////////////////

    if( file_save_output )
        SaveMiniToTxtFile(o_file + ".ref.simplified.txt", liste_mini);

    SaveRawToFile(o_file, liste_mini);

    return 0;
}