#include "minimizer_v3.hpp"
#include "../kmer_list/kmer_counter.hpp"

#include "../front/fastx/read_fastx_file.hpp"
#include "../front/fastx_gz/read_fastx_gz_file.hpp"
#include "../front/fastx_bz2/read_fastx_bz2_file.hpp"
#include "../front/fastx_lz4/read_fastx_lz4_file.hpp"

#include "../front/count_file_lines.hpp"

#include "../hash/CustomMurmurHash3.hpp"

#include "../back/txt/SaveMiniToTxtFile.hpp"
#include "../back/raw/SaveRawToFile.hpp"

#include "../sorting/std_2cores/std_2cores.hpp"
#include "../sorting/std_4cores/std_4cores.hpp"
#include "../sorting/crumsort_2cores/crumsort_2cores.hpp"

#include "../merger/in_file/merger_level_0.hpp"
#include "../minimizer/extract_kmer_v3.hpp"

#include "../tools/CTimer/CTimer.hpp"


#define MEM_UNIT 64ULL
//
//
//
//////////////////////////////////////////////////////////////////////////////////////
//
//
//
inline uint64_t mask_right(const uint64_t numbits)
{
    uint64_t mask = -(numbits >= MEM_UNIT) | ((1ULL << numbits) - 1ULL);
    return mask;
}
//
//
//
//////////////////////////////////////////////////////////////////////////////////////
//
//
//
inline char nucleotide(const int index)
{
    const char table[] = {'A', 'C', 'T', 'G'};
    return table[index];
}
//
//
//
//////////////////////////////////////////////////////////////////////////////////////
//
//
//
void print_kmer(const uint64_t value, const int kmer_size)
{
    for(int i = 0; i < kmer_size; i += 1)
    {
        const int  car =  value >> (2 * kmer_size - 2 - 2 * i);
        const char nuc = nucleotide( car & 0x03 );
        printf("%c", nuc);
    }
}
//
//
//
//////////////////////////////////////////////////////////////////////////////////////
//
//
//
void extract_kmer_v3(
        const std::string& i_file    = "none",
        const std::string& o_file    = "none",
        const std::string& algo      = "crumsort",
        const int  kmer_size         = 27,
        const int  ram_limit_in_MB   = 1024,
        const bool file_save_output  = true,
        const bool verbose_flag      = false,
        const bool file_save_debug   = false
)
{
    CTimer start_gen( true );

    /*
     * Counting the number of SMER in the file (to allocate memory)
     */
    uint64_t max_in_ram = 1024 * 1024* (uint64_t)ram_limit_in_MB / sizeof(uint64_t); // on parle en elements de type uint64_t

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
            (i_file.substr(i_file.find_last_of(".") + 1) == "fna"  ) ||
            (i_file.substr(i_file.find_last_of(".") + 1) == "fa")
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
    uint64_t n_elements = 0;
    uint64_t n_kmers    = 0;
    //
    // Defining counters for statistics
    //

    std::tuple<int, bool> mTuple = reader->next_sequence(seq_value, 4096);

    if( verbose_flag )
        printf("SEQ: %s\n", seq_value);

    //
    //
    //
    int64_t n_sequences = 0;

    //
    // Liste des fichiers temporaires dont on a eu besoin pour générer la liste des minimizers
    // sous contrainte de mémoire (un fichier est créé à chaque fois que l'on dépasse l'espace
    // alloué par l'utilisateur).
    //
    std::vector<std::string> file_list;

    while( std::get<0>(mTuple) != 0 )
    {
#if _debug_core_
        printf("first  = %s (start = %d, new = %d)\n", seq_value, std::get<0>(mTuple), std::get<1>(mTuple) );
#endif

        n_sequences += 1;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // Buffer utilisé pour la fenetre glissante des hash des m-mer
        //
        const uint64_t mask = mask_right(2 * kmer_size); // on masque

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // On initilise le calcul du premier m-mer
        //
        const char* ptr_kmer  = seq_value;  // la position de depart du k-mer
        uint64_t current_mmer = 0;
        uint64_t cur_inv_mmer = 0;
        int cnt = 0;

        for(int x = 0; x < kmer_size - 1; x += 1)
        {
            //
            // Encode les nucleotides du premier k-mer
            //
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer &= mask;

            //
            // On calcule leur forme inversée (pour pouvoir obtenir aussi la forme cannonique)
            //
            cur_inv_mmer >>= 2;
            cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (kmer_size - 1))); // cf Yoann
#if _DEBUG_CURR_
            if( verbose_flag )
                printf(" nuc [%c]: | %16.16llX |\n", ptr_kmer[cnt], current_mmer);
#endif
            cnt          += 1;
        }

        ////////////////////////////////////////////////////////////////////////////////////
        //
        //
        int kmerStartIdx  = 0;
        int nELements     = std::get<0>(mTuple) - kmer_size + 1;
        int isNewSeq      = false;

        //
        // Les sequences de nucleotides peuvent être réparties sur plusieurs lignes
        //
        while( (isNewSeq == false) && (nELements != 0) )
        {

            for(int k_pos = kmerStartIdx; k_pos < nELements; k_pos += 1) // On traite le reste des k-mers
            {
                const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11); // conversion ASCII => 2bits (Yoann)
                current_mmer <<= 2;                                     // fonctionne pour les MAJ et les MIN
                current_mmer |= encoded;
                current_mmer &= mask;
                cur_inv_mmer >>= 2;
                cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (kmer_size - 1))); // cf Yoann

                const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;

                liste_mini[n_elements++] = canon;

                if( verbose_flag )
                {
                    printf("%2d [%c] ", k_pos, ptr_kmer[cnt]);
                    print_kmer(current_mmer, kmer_size); printf(" - ");
                    print_kmer(cur_inv_mmer, kmer_size); printf(" ==> ");
                    print_kmer(canon, kmer_size); printf("\n");
                }

                if( n_elements == max_in_ram )
                {
                    const int64_t MB = n_kmers / 1024 / 1024;
                    printf("- On flush ! (%ld M-kmers)\n", MB);

                    // On est obligé de flush sur le disque les données
                    std::string t_file = o_file + "." + std::to_string( file_list.size() );

                    // On trie les donnnées en memoire
                    //crumsort_prim( liste_mini.data(), n_elements, 9 /*uint64*/ );

                    // On supprime les redondances
                    // int n_elements = VectorDeduplication(liste_mini, n_minizer - 1);
                    //SaveRawToFile(t_file, liste_mini, n_elements);

                    // On memorise le nom du fichier temporaire que l'on a créé
                    //file_list.push_back( t_file );

                    // On reinitialise le buffer de sortie qui est maintenant vide
                    n_elements     = 0;
                }

                cnt     += 1; // on avance le pointeur dans le flux
                n_kmers += 1;
            }

            //
            // On lance le chargement de la ligne suivante
            //
            mTuple        = reader->next_sequence(seq_value, 4096);
            if( verbose_flag )
                printf("std::get<0>(mTuple) == %d | reader->is_eof() == %d\n", std::get<0>(mTuple), reader->is_eof());
            kmerStartIdx  = 0;
            nELements     = std::get<0>(mTuple);
            isNewSeq      = std::get<1>(mTuple);
            cnt           = 0;
        }

        //
        // Si on est arrivé à la fin du fichier alors on se casse!
        //
        if( verbose_flag )
            printf("std::get<0>(mTuple) == %d | reader->is_eof() == %d\n", std::get<0>(mTuple), reader->is_eof());
        if( std::get<0>(mTuple) == 0 ) // aucun nucleotide n'a été obtenu => fin de fichier
            break;
        if( reader->is_eof() == true )    // aucun nucleotide n'a été obtenu => fin de fichier
            break;
    }

    printf("[Parser] Elapsed time %1.3f\n", start_gen.get_time_sec());


    //
    // On a du stocker des données sur le disque dur en cours de route... On finit ICI et MAINTENANT !
    //
    if( file_list.size() != 0 )
    {
#if _debug_mode_
        printf("[Minimizer_v3] Executing fine merging loop [%s, %d]\n", __FILE__, __LINE__);
#endif

        //
        // On flush le dernier lot de données avant de débuter la fusion (c plus homogène)
        //
//      SaveRawToFile(o_file + ".non-sorted." + std::to_string( file_list.size() ), liste_mini, n_minizer);

        std::string t_file = o_file + "." + std::to_string( file_list.size() );
        crumsort_prim( liste_mini.data(), n_elements, 9 /*uint64*/ );

//      SaveRawToFile(o_file + ".sorted." + std::to_string( file_list.size() ), liste_mini, n_minizer);

//        int n_elements = VectorDeduplication(liste_mini, n_minizer);
        SaveRawToFile(t_file, liste_mini, n_elements);

        file_list.push_back( t_file );
#if _debug_mode_
        printf("[Minimizer_v3] - Flushed to SSD drive [%s, %d]\n", __FILE__, __LINE__);
#endif

        int name_c = file_list.size();
        while(file_list.size() > 1)
        {
            const std::string t_file = o_file + "." + std::to_string( name_c++ );
            merge_level_0(file_list[0], file_list[1], t_file);
            std::remove(file_list[0].c_str() ); // delete file
            std::remove(file_list[1].c_str() ); // delete file
            file_list.erase(file_list.begin());
            file_list.erase(file_list.begin());
            file_list.push_back( t_file );
        }

#if _debug_mode_
        printf("[Minimizer_v3] - Renaming final file to %s [%s, %d]\n", o_file.c_str(), __FILE__, __LINE__);
#endif
        std::rename(file_list[file_list.size()-1].c_str(), o_file.c_str());

        delete reader;
        return;
    }



    // On vient de finir le traitement d'une ligne. On peut en profiter pour purger le
    // buffer de sortie sur le disque dur apres l'avoir purgé des doublons et l'avoir
    // trié...
    CTimer start_resize( true );
    liste_mini.resize(n_elements);
    printf("[Resize] Elapsed time %1.3f\n", start_resize.get_time_sec());

    if( verbose_flag == true ) {
        printf("(II)\n");
        printf("(II) Launching the sorting step\n");
        printf("(II) - Sorting algorithm = %s\n", algo.c_str());
        printf("(II) - Number of samples = %ld\n", liste_mini.size());
    }

    CTimer start_sort( true );
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
    printf("[Sortng] Elapsed time %1.3f\n", start_sort.get_time_sec());

    //
    // En regle général on save le résultat sauf lorsque l'on fait du benchmarking
    //

    if( file_save_debug )
    {
        SaveMiniToTxtFile_v2(o_file + ".sorted.txt", liste_mini);
        SaveRawToFile       (o_file + ".hash", liste_mini);
    }

    //
    // On stoque à l'indentique les données que l'on vient lire. Cette étape est uniquement utile
    // pour du debug
    //
    if( verbose_flag == true ) {
        printf("(II)\n");
        printf("(II) Vector deduplication step\n");
    }

    //
    // On cree le tableau qui va contenir les occurences
    //
    std::vector<int> liste_cntr( liste_mini.size() );

    //
    // On lance l'étape de déduplication & comptage des k-mer
    //
    CTimer start_cnt( true );
    kmer_counter(liste_mini, liste_cntr);
    printf("[Countg] Elapsed time %1.3f\n", start_cnt.get_time_sec());

    //
    // On affiche les résultats
    //
    if( verbose_flag )
    {
        for(size_t i = 0; i < liste_mini.size(); i += 1)
        {
            printf("%2ld : ", i);
            print_kmer(liste_mini[i], kmer_size);
            printf(" - %6d\n", liste_cntr[i]);
        }
    }

    printf("Stats:\n");
    printf("  No. of k-mers below min. threshold :            0\n");
    printf("  No. of k-mers above max. threshold :            0\n");
    printf("  No. of unique k-mers               : %12zu\n",  liste_mini.size());
    printf("  No. of unique counted k-mers       : %12ld\n", liste_mini.size());
    printf("  Total no. of k-mers                : %12ld\n", n_kmers);
    printf("  Total no. of sequences             : %12ld\n", n_sequences);
    printf("  Total no. of super-k-mers          :            0\n");

    printf("Elapsed time %1.3f\n", start_gen.get_time_sec());

    if( file_save_debug )
        SaveMiniToTxtFile_v2(o_file + ".txt", liste_mini);

    if( file_save_output )
        SaveRawToFile(o_file, liste_mini);

    delete reader;
}
