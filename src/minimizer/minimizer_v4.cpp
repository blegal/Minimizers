#include "minimizer_v3.hpp"
#include "../kmer_list/smer_deduplication.hpp"

#include <iostream>

#include "../front/fastx/read_fastx_ATCG_only.hpp"
#include "../front/fastx_gz/read_fastx_gz_ATCG_only.hpp"
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

#define MEM_UNIT 64ULL

#define _debug_ 0
#define _murmurhash_

inline uint64_t mask_right(const uint64_t numbits)
{
    uint64_t mask = -(numbits >= MEM_UNIT) | ((1ULL << numbits) - 1ULL);
    return mask;
}
//
//
//

void minimizer_processing_v4(
        const std::string& i_file    = "none",
        const std::string& o_file    = "none",
        const std::string& algo      = "crumsort",
        const int  ram_limit_in_MB   = 1024,
        const bool file_save_output  = true,
        const bool verbose_flag      = false,
        const bool file_save_debug   = false,
        const int kmer = 31,
        const int mmer = 19
)
{
    bool verbose = false;
    if (verbose) std::cerr << "minimizer_processing_v4" << std::endl;
    /*
     * Counting the number of SMER in the file (to allocate memory)
     */
    uint64_t max_in_ram = 1024 * 1024* (uint64_t)ram_limit_in_MB / sizeof(uint64_t); // on parle en elements de type uint64_t

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
        reader = new read_fastx_gz_ATCG_only(i_file);
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
        reader = new read_fastx_ATCG_only(i_file);
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
    uint64_t n_minizer     = 0;
    int n_skipper     = 0;

    std::tuple<int, bool> mTuple = reader->next_sequence(seq_value, 4096);

    //
    // Liste des fichiers temporaires dont on a eu besoin pour générer la liste des minimizers
    // sous contrainte de mémoire (un fichier est créé à chaque fois que l'on dépasse l'espace
    // alloué par l'utilisateur).
    //
    std::vector<std::string> file_list;

    while( std::get<0>(mTuple) != 0 )
    {
        while (std::get<0>(mTuple) < kmer){
            //seq was cut because of "N" characters
            mTuple = reader->next_sequence(seq_value, 4096);

            //check if file ended on "N" characters
            if( std::get<0>(mTuple) == 0 ) // aucun nucleotide n'a été obtenu => fin de fichier
            break;
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // Buffer utilisé pour la fenetre glissante des hash des m-mer
        //
        uint64_t buffer[z + 1]; // nombre de m-mer/k-mer
        for(int x = 0; x <= z; x += 1)
            buffer[x] = UINT64_MAX;

        uint64_t mask = mask_right(2 * mmer); // on masque

        // PREMIER M-MER
        if (verbose) std::cout << "PREMIER M-MER" << std::endl;
        const char* ptr_kmer  = seq_value;  // la position de depart du k-mer
        uint64_t current_mmer = 0;
        uint64_t cur_inv_mmer = 0;
        int cnt = 0;

        for(int x = 0; x < mmer - 1; x += 1)
        {
            if (verbose) std::cout << cnt << " " << ptr_kmer[cnt] << std::endl;
            // Encode les nucleotides du premier s-mer
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer &= mask;
            cur_inv_mmer >>= 2;
            cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); // cf Yoann
            cnt          += 1;
        }

        
        // PREMIER K-MER
        if (verbose) std::cout << "PREMIER K-MER" << std::endl;
        uint64_t minv = UINT64_MAX;
        for(int m_pos = 0; m_pos <= z; m_pos += 1)
        {
            if (verbose) std::cout << cnt << " " << ptr_kmer[cnt] << std::endl;
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
            buffer[m_pos]         = s_hash; // on memorise le hash du mmer
            minv                  = (s_hash < minv) ? s_hash : minv;
            cnt                  += 1; // on avance dans la sequence de nucleotides
        }

        //1st kmer done
        if( n_minizer == 0 ){
            liste_mini[n_minizer++] = minv;
            if (verbose) std::cout << "add minmer " << minv << std::endl;
        }else if( liste_mini[n_minizer-1] != minv ){
            liste_mini[n_minizer++] = minv;
            if (verbose) std::cout << "add minmer " << minv << std::endl;
        }else{
            n_skipper += 1;
        }

        //ALL OTHER KMERS
        if (verbose) std::cout << "OTHER KMERS" << std::endl;
        int kmerStartIdx  = 1;
        int nELements     = std::get<0>(mTuple) - kmer + 1;
        int isNewSeq      = false;

        while( isNewSeq == false )
        {
            for(int k_pos = kmerStartIdx; k_pos < nELements; k_pos += 1) // On traite le reste des k-mers
            {
                if (verbose) std::cout << cnt << " " << ptr_kmer[cnt] << std::endl;
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

                if( minv == buffer[0] ) // si minv est un m-mer sortant, trouve le nveau min
                {
                    minv = s_hash;
                    for(int p = 0; p < z; p += 1) {
                        const uint64_t value = buffer[p + 1];
                        minv = (minv < value) ? minv : value;
                        buffer[p] = value;
                    }
                    buffer[z] = s_hash; // on memorise le hash du dernier m-mer
                }else{
                    for(int p = 0; p < z; p += 1) {
                        buffer[p] = buffer[p+1]; //TODO: pas moyen d'eviter les deplacements de donnée ici ?
                    }
                    buffer[z] = s_hash; // on memorise le hash du dernier m-mer
                }

                
                if( liste_mini[n_minizer-1] != minv ){
                    liste_mini[n_minizer++] = minv;
                    if (verbose) std::cout << "add minmer " << minv << std::endl;

                    if( n_minizer >= (max_in_ram - 2) )
                    {
                        // On est obligé de flush sur le disque les données
                        std::string t_file = o_file + "." + std::to_string( file_list.size() );


                        // On trie les donnnées en memoire
                        crumsort_prim( liste_mini.data(), n_minizer - 1, 9 /*uint64*/ );


                        // On supprime les redondances
                        int n_elements = smer_deduplication(liste_mini, n_minizer - 1);

                        SaveRawToFile(t_file, liste_mini, n_elements - 1);

                        file_list.push_back( t_file );
                        liste_mini[0] = liste_mini[n_minizer-1]; //Patch
                        n_minizer     = 1;
                    }
                }else{
                    n_skipper += 1;
                }

                kmer_cnt += 1; // on a traité un kmer de plus
                cnt      += 1; // on avance le pointeur dans le flux
                if( minv == 0 ) exit( 0 );
            }

            // On lance le chargement du buffer suivant
            mTuple        = reader->next_sequence(seq_value, 4096);
            kmerStartIdx  = 0;
            nELements     = std::get<0>(mTuple);
            isNewSeq      = std::get<1>(mTuple);
            cnt           = 0;
        }

        // Si on est arrivé à la fin du fichier alors on se casse!
        if( std::get<0>(mTuple) == 0 ) // aucun nucleotide n'a été obtenu => fin de fichier
            break;
    }

    // On a du stocker des données sur le disque dur en cours de route... On finit ICI et MAINTENANT !
    if( file_list.size() != 0 )
    {
        std::string t_file = o_file + "." + std::to_string( file_list.size() );
        crumsort_prim( liste_mini.data(), n_minizer, 9 /*uint64*/ );

        int n_elements = smer_deduplication(liste_mini, n_minizer);
        SaveRawToFile(t_file, liste_mini, n_elements);

        file_list.push_back( t_file );

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
        std::rename(file_list[file_list.size()-1].c_str(), o_file.c_str());

        delete reader;
        return;
    }



    // NO SAVE DURING PROCESSING, SAVE HERE

    liste_mini.resize(n_minizer);

    if( file_save_debug ){
        SaveMiniToTxtFile_v2(o_file + ".non-sorted-v2.txt", liste_mini);
    }

    if( verbose_flag == true ) {
        printf("(II)\n");
        printf("(II) Launching the sorting step\n");
        printf("(II) - Sorting algorithm = %s\n", algo.c_str());
        printf("(II) - Number of samples = %ld\n", liste_mini.size());
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

    if( file_save_debug )
    {
        SaveMiniToTxtFile_v2(o_file + ".sorted.txt", liste_mini);
        SaveRawToFile   (o_file + ".hash", liste_mini);
    }

    if( verbose_flag == true ) {
        printf("(II)\n");
        printf("(II) Vector deduplication step\n");
    }

    smer_deduplication( liste_mini );

    if( file_save_debug ){
        SaveMiniToTxtFile_v2(o_file + ".txt", liste_mini);
    }

    if( file_save_output ){
        SaveRawToFile(o_file, liste_mini);
    }

    delete reader;

}





void minimizer_processing_stats_only(
        const std::string& i_file,
        std::vector<uint64_t>& results
    )
{
    bool verbose = false;
    if (verbose) std::cerr << "minimizer_processing_stats_only" << std::endl;

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
        reader = new read_fastx_gz_ATCG_only(i_file);
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
        reader = new read_fastx_ATCG_only(i_file);
    }
    else
    {
        printf("(EE) File extension is not supported (%s)\n", i_file.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }


    uint64_t kmer_cnt  = 0;
    uint64_t n_minizer = 0;
    uint64_t last_minmer_seen = 0;
    uint64_t bp_cnt    = 0;
    uint64_t n_skipper      = 0;

    std::tuple<int, bool> mTuple = reader->next_sequence(seq_value, 4096);

    //
    // Liste des fichiers temporaires dont on a eu besoin pour générer la liste des minimizers
    // sous contrainte de mémoire (un fichier est créé à chaque fois que l'on dépasse l'espace
    // alloué par l'utilisateur).
    //
    std::vector<std::string> file_list;

    while( std::get<0>(mTuple) != 0 )
    {
        while (std::get<0>(mTuple) < kmer){
            //seq was cut because of "N" characters
            mTuple = reader->next_sequence(seq_value, 4096);

            //check if file ended on "N" characters
            if( std::get<0>(mTuple) == 0 ) // aucun nucleotide n'a été obtenu => fin de fichier
            break;
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // Buffer utilisé pour la fenetre glissante des hash des m-mer
        //
        uint64_t buffer[z + 1]; // nombre de m-mer/k-mer
        for(int x = 0; x <= z; x += 1)
            buffer[x] = UINT64_MAX;

        uint64_t mask = mask_right(2 * mmer); // on masque

        // PREMIER M-MER
        if (verbose) std::cout << "PREMIER M-MER" << std::endl;
        const char* ptr_kmer  = seq_value;  // la position de depart du k-mer
        uint64_t current_mmer = 0;
        uint64_t cur_inv_mmer = 0;
        int cnt = 0;

        for(int x = 0; x < mmer - 1; x += 1)
        {
            bp_cnt += 1;
            if (verbose) std::cout << cnt << " " << ptr_kmer[cnt] << std::endl;
            // Encode les nucleotides du premier s-mer
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer &= mask;
            cur_inv_mmer >>= 2;
            cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); // cf Yoann
            cnt          += 1;
        }

        
        // PREMIER K-MER
        if (verbose) std::cout << "PREMIER K-MER" << std::endl;
        uint64_t minv = UINT64_MAX;
        for(int m_pos = 0; m_pos <= z; m_pos += 1)
        {
            bp_cnt += 1;
            if (verbose) std::cout << cnt << " " << ptr_kmer[cnt] << std::endl;
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
            buffer[m_pos]         = s_hash; // on memorise le hash du mmer
            minv                  = (s_hash < minv) ? s_hash : minv;
            cnt                  += 1; // on avance dans la sequence de nucleotides
        }

        //1st kmer done
        kmer_cnt += 1;

        if( n_minizer == 0 ){
            n_minizer++;
        }else if( last_minmer_seen != minv ){
            last_minmer_seen = minv;
            n_minizer++;
        }else{
            n_skipper += 1;
        }

        //ALL OTHER KMERS
        if (verbose) std::cout << "OTHER KMERS" << std::endl;
        int kmerStartIdx  = 1;
        int nELements     = std::get<0>(mTuple) - kmer + 1;
        int isNewSeq      = false;

        while( isNewSeq == false )
        {
            for(int k_pos = kmerStartIdx; k_pos < nELements; k_pos += 1) // On traite le reste des k-mers
            {
                bp_cnt += 1;
                if (verbose) std::cout << cnt << " " << ptr_kmer[cnt] << std::endl;
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

                if( minv == buffer[0] ) // si minv est un m-mer sortant, trouve le nveau min
                {
                    minv = s_hash;
                    for(int p = 0; p < z; p += 1) {
                        const uint64_t value = buffer[p + 1];
                        minv = (minv < value) ? minv : value;
                        buffer[p] = value;
                    }
                    buffer[z] = s_hash; // on memorise le hash du dernier m-mer
                }else{
                    for(int p = 0; p < z; p += 1) {
                        buffer[p] = buffer[p+1]; //TODO: pas moyen d'eviter les deplacements de donnée ici ?
                    }
                    buffer[z] = s_hash; // on memorise le hash du dernier m-mer
                }

                
                if( last_minmer_seen != minv ){
                    last_minmer_seen = minv;
                    n_minizer++;
                }else{
                    n_skipper += 1;
                }

                kmer_cnt += 1; // on a traité un kmer de plus
                cnt      += 1; // on avance le pointeur dans le flux
                if( minv == 0 ) exit( 0 );
            }

            // On lance le chargement du buffer suivant
            mTuple        = reader->next_sequence(seq_value, 4096);
            kmerStartIdx  = 0;
            nELements     = std::get<0>(mTuple);
            isNewSeq      = std::get<1>(mTuple);
            cnt           = 0;
        }

        // Si on est arrivé à la fin du fichier alors on se casse!
        if( std::get<0>(mTuple) == 0 ) // aucun nucleotide n'a été obtenu => fin de fichier
            break;
    }


    results.push_back( bp_cnt );
    results.push_back( kmer_cnt );
    results.push_back( n_minizer );
    
    delete reader;

}
