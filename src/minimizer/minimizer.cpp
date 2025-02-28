#include "minimizer.hpp"
#include "../kmer_list/smer_deduplication.hpp"

#include "../progress/progressbar.h"

#include "../front/fastx/read_fastx_file.hpp"
#include "../front/fastx_bz2/read_fastx_bz2_file.hpp"

#include "../front/count_file_lines.hpp"

#include "../front/read_k_value.hpp"

#include "../hash//CustomMurmurHash3.hpp"

#include "../back/txt/SaveMiniToTxtFile.hpp"
#include "../back/raw/SaveRawToFile.hpp"

#include "../sorting/std_2cores/std_2cores.hpp"
#include "../sorting/std_4cores/std_4cores.hpp"
#include "../sorting/crumsort_2cores/crumsort_2cores.hpp"

#define MEM_UNIT 64ULL

inline uint64_t mask_right(const uint64_t numbits)
{
    uint64_t mask = -(numbits >= MEM_UNIT) | ((1ULL << numbits) - 1ULL);
    return mask;
}

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
    file_reader* reader;
    if (i_file.substr(i_file.find_last_of(".") + 1) == "bz2")
    {
        reader = new read_fastx_bz2_file(i_file);
    }
    else if(i_file.substr(i_file.find_last_of(".") + 1) == "fastx")
    {
        reader = new read_fastx_file(i_file);
    }
    else
    {
        printf("(EE) File extension is not supported (%s)\n", i_file.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

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
        bool not_oef = reader->next_sequence(seq_value);
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


            uint64_t tab[2];
            CustomMurmurHash3_x64_128<8> ( &canon, 42, tab );
            const uint64_t s_hash = tab[0];

            //
            //

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
        printf("(+)-  min = | %16.16llX |\n", minv);
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

            uint64_t tab[2];
            CustomMurmurHash3_x64_128<8> ( &canon, 42, tab );
            const uint64_t s_hash = tab[0];

            minv                  = (s_hash < minv) ? s_hash : minv;

            if( minv == buffer[0] )
            {
                minv = s_hash;
//              for(int p = 0; p < z + 1; p += 1) {
                for(int p = 0; p < z; p += 1) { // < z car on fait p+1
                    const uint64_t value = buffer[p+1];
                    minv = (minv < value) ? minv : value;
                    buffer[p] = value;
                }
                buffer[z] = s_hash; // on memorise le hash du dernier m-mer
            }else{
//                for(int p = 0; p < z + 1; p += 1) {
                for(int p = 0; p < z; p += 1) { // < z car on fait p+1
                    buffer[p] = buffer[p + 1];
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
            printf("(+)   min = | %16.16llX |\n", minv);

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

        // On vient de finir le traitement d'une ligne. On peut en profiter pour purger le
        // buffer de sortie sur le disque dur apres l'avoir purgé des doublons et l'avoir
        // trié...

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

//    double start_time = omp_get_wtime();
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
//    if( verbose_flag == true ) {
//        double end_time = omp_get_wtime();
//        printf("(II) - Execution time    = %f\n", end_time - start_time);
//    }

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
        SaveRawToFile   (o_file + ".hash", liste_mini);
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

    smer_deduplication( liste_mini );

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
        SaveRawToFile(o_file, liste_mini);
    }

    delete reader;

}
