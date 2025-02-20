#include "minimizer_v2.hpp"
#include "../kmer_list/smer_deduplication.hpp"

#include "../front/fastx/read_fastx_file.hpp"
#include "../front/fastx_gz/read_fastx_gz_file.hpp"
#include "../front/fastx_bz2/read_fastx_bz2_file.hpp"
#include "../front/fastx_lz4/read_fastx_lz4_file.hpp"

#include "../hash/CustomMurmurHash3.hpp"

#include "../back/txt/SaveMiniToTxtFile.hpp"
#include "../back/raw/SaveRawToFile.hpp"

#include "../sorting/std_2cores/std_2cores.hpp"
#include "../sorting/std_4cores/std_4cores.hpp"
#include "../sorting/crumsort_2cores/crumsort_2cores.hpp"

#define MEM_UNIT 64ULL

#define _debug_ 0

inline uint64_t mask_right(const uint64_t numbits)
{
    uint64_t mask = -(numbits >= MEM_UNIT) | ((1ULL << numbits) - 1ULL);
    return mask;
}
//
//
//

void minimizer_processing_v2(
        const std::string& i_file    = "none",
        const std::string& o_file    = "none",
        const std::string& algo      = "crumsort",
        const int  ram_limit_in_MB   = 1024,
        const bool file_save_output  = true,
        const bool verbose_flag      = false,
        const bool file_save_debug   = false
)
{

    /*
     * Counting the number of SMER in the file (to allocate memory)
     */
    int max_in_ram = 1024 * 1024* ram_limit_in_MB / sizeof(uint64_t); // on parle en elements de type uint64_t

    /*
     * Reading the K value from the first file line
     */
    const int kmer = 31;
    const int mmer = 19;
    const int z    = kmer - mmer;

    //
    // On cree le buffer qui va nous permettre de recuperer les
    // nucleotides depuis le fichier
    //

    char seq_value[4096];
    bzero(seq_value, 4096);

    //
    // Allocating the object that performs fast file parsing
    //
    file_reader* reader;
    if (i_file.substr(i_file.find_last_of(".") + 1) == "bz2")
    {
        reader = new read_fastx_bz2_file(i_file);
    }
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "gz")
    {
        reader = new read_fastx_gz_file(i_file);
    }
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "lz4")
    {
        reader = new read_fastx_lz4_file(i_file);
    }
    else if(
            (i_file.substr(i_file.find_last_of(".") + 1) == "fastx") ||
            (i_file.substr(i_file.find_last_of(".") + 1) == "fasta") ||
            (i_file.substr(i_file.find_last_of(".") + 1) == "fastq") ||
            (i_file.substr(i_file.find_last_of(".") + 1) == "fna")
            )
    {
        reader = new read_fastx_file(i_file);
    }
    else
    {
        printf("(EE) File extension is not supported (%s)\n", i_file.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    //
    // Allocating the output memory buffer
    //

    std::vector<uint64_t> liste_mini( max_in_ram );

    //
    // Defining counters for statistics
    //

    uint32_t kmer_cnt = 0;
    int n_minizer     = 0;
    int n_skipper     = 0;
    int n_lines       = 0;

    std::tuple<int, bool> mTuple = reader->next_sequence(seq_value, 4096);

    while( std::get<0>(mTuple) != 0 )
    {
#if _debug_core_
        printf("first  = %s (start = %d, new = %d)\n", seq_value, std::get<0>(mTuple), std::get<1>(mTuple) );
#endif

        n_lines += 1; // on compte les séquences

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // Buffer utilisé pour la fenetre glissante des hash des m-mer
        //
        uint64_t buffer[z + 1]; // nombre de m-mer/k-mer
        for(int x = 0; x <= z; x += 1)
            buffer[x] = UINT64_MAX;

        uint64_t mask = mask_right(2 * mmer); // on masque

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // On initilise le calcul du premier m-mer
        //
        const char* ptr_kmer  = seq_value;  // la position de depart du k-mer
        uint64_t current_mmer = 0;
        uint64_t cur_inv_mmer = 0;
        int cnt = 0;

        for(int x = 0; x < mmer - 1; x += 1)
        {
            //
            // Encode les nucleotides du premier s-mer
            //
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer &= mask;

            //
            // On calcule leur forme inversée (pour pouvoir obtenir aussi la forme cannonique)
            //
            cur_inv_mmer >>= 2;
            cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); // cf Yoann
#if _DEBUG_CURR_
            if( verbose_flag )
                printf(" nuc [%c]: | %16.16llX |\n", ptr_kmer[cnt], current_mmer);
#endif
            cnt          += 1;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // On traite le premier k-mer. Pour cela on calcule ses differents s-mer afin d'en extraire la
        // valeur minimum.
        //
#if _DEBUG_CURR_
        if( verbose_flag )
        {
            printf(" k: ");
            for(int x = 0; x < kmer; x += 1) printf("%c", ptr_kmer[x]);
            printf("\n");
        }
#endif
        ////////////////////////////////////////////////////////////////////////////////////

#if _debug_core_
        printf("   curr-m-mer       | curr-m-mer-inv   | cannonic-hash    |          hash    |\n");
#endif
        uint64_t minv = UINT64_MAX;
        for(int m_pos = 0; m_pos <= z; m_pos += 1)
        {
            // On calcule les m-mers
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11); // conversion ASCII => 2bits (Yoann)
            current_mmer <<= 2;
            current_mmer |= encoded;
            current_mmer &= mask;
            cur_inv_mmer >>= 2;
            cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); // cf Yoann

            const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;

            uint64_t tab[2];
            CustomMurmurHash3_x64_128<8> ( &canon, 42, tab );
            const uint64_t s_hash = tab[0];
            //
#if _debug_core_
            printf(" - %16.16llX | %16.16llX | %16.16llX | %16.16llX |\n", current_mmer, cur_inv_mmer, canon, s_hash);
#endif
            //
            buffer[m_pos]         = s_hash; // on memorise le hash du mmer
            minv                  = (s_hash < minv) ? s_hash : minv;
            cnt                  += 1; // on avance dans la sequence de nucleotides
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // On memorise la valeur du minimiser du premier kmer de la sequence dans le buffer de sortie
        //
#if _debug_core_
        printf("(+)1  min = | %16.16llX |\n", minv);
#endif
            if( n_minizer == 0 ){
                liste_mini[n_minizer++] = minv;
#if _debug_core_
            printf("   - pushed (%d)\n", n_minizer);
#endif
        }else if( liste_mini[n_minizer-1] != minv ){
            liste_mini[n_minizer++] = minv;
#if _debug_core_
            printf("   - pushed (%d)\n", n_minizer);
#endif
        }else{
            n_skipper += 1;
#if _debug_core_
            printf("   - skipped (%d)\n", n_minizer);
#endif
        }

        kmer_cnt += 1;

        ////////////////////////////////////////////////////////////////////////////////////
        //
        // On va generer tous les m-mer à partir des k-mers. Attention toutefois, leur
        // nombre varie en fonction du bout de la sequence que l'on traite:
        // - premier morceau, il y en a N nucleotides - sizeof(mmer) + 1 car il a fallu
        //   amorcer l'encodage des nucleotides
        // - dans les morceaux suivants il y en a N a chaque fois !
        //
        int kmerStartIdx  = 1;
        int nELements     = std::get<0>(mTuple) - kmer + 1;
        int isNewSeq      = false;

        //
        // Les sequences de nucleotides peuvent être réparties sur plusieurs lignes
        //
        while( isNewSeq == false )
        {
#if _debug_core_
            printf("   curr-m-mer       | curr-m-mer-inv   | cannonic-hash    |          hash    |\n");
#endif

            for(int k_pos = kmerStartIdx; k_pos < nELements; k_pos += 1) // On traite le reste des k-mers
            {
                const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11); // conversion ASCII => 2bits (Yoann)
                current_mmer <<= 2;                                     // fonctionne pour les MAJ et les MIN
                current_mmer |= encoded;
                current_mmer &= mask;
                cur_inv_mmer >>= 2;
                cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); // cf Yoann

                const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;

                uint64_t tab[2];
                CustomMurmurHash3_x64_128<8> ( &canon, 42, tab );
                const uint64_t s_hash = tab[0];
                minv                  = (s_hash < minv) ? s_hash : minv;

#if _debug_core_
                printf(" - %16.16llX | %16.16llX | %16.16llX | %16.16llX |\n", current_mmer, cur_inv_mmer, canon, s_hash);
#endif

                if( minv == buffer[0] )
                {
                    minv = s_hash;
                    for(int p = 0; p < z; p += 1) {
                        const uint64_t value = buffer[p + 1];
                        minv = (minv < value) ? minv : value;
                        buffer[p] = value;
                    }
                    buffer[z] = s_hash; // on memorise le hash du dernier m-mer
#if _debug_core_
                    for(int p = 0; p < z; p += 1)
                        printf(" --------- minv = %16.16llX / %16.16llX\n", minv, buffer[p]);
                    printf(" -2minv = %16.16llX (1)\n", minv);
#endif
                }else{
                    for(int p = 0; p < z; p += 1) {
                        buffer[p] = buffer[p+1];
                    }
                    buffer[z] = s_hash; // on memorise le hash du dernier m-mer
#if _debug_core_
                    for(int p = 0; p < z + 1; p += 1)
                        printf(" --------- minv = %16.16llX / %16.16llX\n", minv, buffer[p]);
#endif
                }

                ////////////////////////////////////////////////////////////////////////////////////

                if( liste_mini[n_minizer-1] != minv ){
#if _debug_core_
                    printf("(+)   min = | %16.16llX |\n", minv);
#endif
                    liste_mini[n_minizer++] = minv;
                    if( n_minizer >= (max_in_ram - 2) )
                    {
                        //
                        // On est obligé de resize le vecteur car on n'a plus de place
                        //
                        max_in_ram += (max_in_ram >> 2);    // on rajoute 50%
                        liste_mini.resize( max_in_ram );
                    }
#if _debug_core_
                    printf("   - pushed (%d)\n", n_minizer);
#endif
                }else{
#if _debug_core_
                    printf("(+)  skip = | %16.16llX |\n", minv);
#endif
                    n_skipper += 1;
#if _debug_core_
                    printf("   - skipped (%d)\n", n_minizer);
#endif
                }

                kmer_cnt += 1; // on a traité un kmer de plus
                cnt      += 1; // on avance le pointeur dans le flux
// DEBUG !
                if( minv == 0 ) exit( 0 );
// FIN DEBUG
            }

            //
            // On lance le chargement de la ligne suivante
            //
            mTuple        = reader->next_sequence(seq_value, 4096);
            kmerStartIdx  = 0;
            nELements     = std::get<0>(mTuple);
            isNewSeq      = std::get<1>(mTuple);
            cnt           = 0;
#if _debug_core_

            if( isNewSeq == false )
                printf("+ next = %s (start = %d, new = %d)\n", seq_value, nELements, isNewSeq );
            else
                printf("+ End of seq detected\n");
#endif
            //
            // On va regarder si le nombre de données présentes dans le buffer de sortie
            // dépasse une borne MAX, si oui on va flusher sur le disque dur les données
            // temporaires apres les avoir triées et purgées des doublons.
            //

            // TODO !
        }

        //
        // Si on est arrivé à la fin du fichier alors on se casse!
        //
        if( std::get<0>(mTuple) == 0 ) // aucun nucleotide n'a été obtenu => fin de fichier
            break;
        if( reader->is_eof() == true )    // aucun nucleotide n'a été obtenu => fin de fichier
            break;

    }

    // On vient de finir le traitement d'une ligne. On peut en profiter pour purger le
    // buffer de sortie sur le disque dur apres l'avoir purgé des doublons et l'avoir
    // trié...

    liste_mini.resize(n_minizer);

    if( verbose_flag == true ) {
        printf("(II) Minimizer generation pass\n");
        printf("(II) - Number of ADN sequences  : %10d\n", n_lines);
        printf("(II) - Number of k-mer          : %10u\n", kmer_cnt);
        printf("(II) - Number of skipped minizr : %10d\n", n_skipper);
        printf("(II) - Number of minimizers     : %10zu\n", liste_mini.size());
        printf("(II) - Memory occupancy         : %10lu MB\n", (liste_mini.size() * sizeof(uint64_t)) / 1024 / 1024);
        printf("(II)\n");
    }

//  SaveRawToFile(o_file + ".non-sorted.0", liste_mini, 268435454);

    //
    // On stoque à l'indentique les données que l'on vient lire. Cette étape est uniquement utile
    // pour du debug
    //
//  SaveRawToFile(o_file + ".non-sorted", liste_mini);

    if( file_save_debug ){
        SaveMiniToTxtFile_v2(o_file + ".non-sorted-v2.txt", liste_mini);
    }

    if( verbose_flag == true ) {
        printf("(II) Launching the sorting step\n");
        printf("(II) - Sorting algorithm = %s\n", algo.c_str());
        printf("(II) - Number of samples = %ld\n", liste_mini.size());
        printf("(II)\n");
    }

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
    uint64_t before =  liste_mini.size();
    smer_deduplication( liste_mini );
    uint64_t after  =  liste_mini.size();
    uint64_t diffr  =  before - after;

    if( verbose_flag == true )
    {
        printf("(II) Vector deduplication step\n");
        printf("(II) - #m-mer before = %llu\n", before);
        printf("(II) - #m-mer after  = %llu\n", after);
        printf("(II) - #m-mer saving = %llu\n", diffr);
        printf("(II)\n");
    }

    if( file_save_debug ){
        SaveMiniToTxtFile_v2(o_file + ".txt", liste_mini);
    }

    if( file_save_output ){
        SaveRawToFile(o_file, liste_mini);
    }

    delete reader;

}
