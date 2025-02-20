#include "extract_kmer_v4.hpp"
#include "../kmer_list/kmer_counter.hpp"

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

#include "../files/stream_reader_library.hpp"


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
extern void print_kmer(const uint64_t value, const int kmer_size);
//
//
//
//////////////////////////////////////////////////////////////////////////////////////
//
//
//
//#define _debug_fsm_

void extract_kmer_v4(
        const std::string& i_file    = "none",
        const std::string& o_file    = "none",
        const std::string& algo      = "crumsort",
        const int  kmer_size         = 27,
        const int  ram_limit_in_MB   = 1024,
        const int  ram_limit_ou_MB   = 1024,
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
    uint64_t max_ou_ram = 1024 * 1024* (uint64_t)ram_limit_ou_MB / sizeof(uint64_t); // on parle en elements de type uint64_t

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // Buffer utilisé pour la fenetre glissante des hash des m-mer
    //
    const uint64_t mask = mask_right(2 * kmer_size); // on masque

    //
    // On cree le buffer qui va nous permettre de recuperer les
    // nucleotides depuis le fichier
    //

    char* i_buffer = new char[ram_limit_in_MB];

    //
    // Allocating the object that performs fast file parsing
    //
    stream_reader* reader = stream_reader_library::allocate( i_file );
    if( reader == nullptr )
    {
        printf("(EE) File extension is not supported (%s)\n", i_file.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    //
    // Allocating the output memory buffer
    //

    std::vector<uint64_t> liste_mini( max_ou_ram );
    int64_t stream_ptr  = 0;
    int64_t n_elements  = 0;
    int64_t n_kmers     = 0;
    int64_t n_non_valid = 0;
    int64_t n_sequences = 0;

    uint64_t current_mmer = 0;
    uint64_t cur_inv_mmer = 0;
    uint64_t canonnic     = 0;

    int64_t bytes_read    = 0;

    //
    // Defining counters for statistics
    //
    int64_t n_nucleotides = reader->read( i_buffer, sizeof(char), ram_limit_in_MB);
    bytes_read   += n_nucleotides;
    bool is_ended = (n_nucleotides != ram_limit_in_MB);
#ifdef _debug_fsm_
    printf("n_nucleotides = %lld\n", n_nucleotides);
#endif
    //
    // Liste des fichiers temporaires dont on a eu besoin pour générer la liste des minimizers
    // sous contrainte de mémoire (un fichier est créé à chaque fois que l'on dépasse l'espace
    // alloué par l'utilisateur).
    //
    std::vector<std::string> file_list;

    if( i_buffer[stream_ptr] == '>' ){
        goto jump_header;
    }else{
        goto kernel_init;
    }

    jump_header:
        //
        //
        //
#ifdef _debug_fsm_
        printf("FSM (jump_header)\n");
#endif
        //
        // on recharge les data si cela est necessaire
        //
        if( (stream_ptr == n_nucleotides) && (is_ended == false) )
        {
            printf("Reload header\n");
            n_nucleotides = reader->read( i_buffer, sizeof(char), ram_limit_in_MB);
            bytes_read   += n_nucleotides;
            is_ended      = (n_nucleotides != ram_limit_in_MB);
            stream_ptr    = 0;
            goto kernel_loop;   // on continue le traitement des données
        }

        for(int64_t ptr = stream_ptr; ptr < n_nucleotides; ptr += 1)
        {
            if( i_buffer[stream_ptr++] == '\n' )
                break;
        }
#ifdef _debug_fsm_
        printf("position de fin d'entete = %lld\n", stream_ptr);
#endif
        // on est a la fin du fichier alors on quitte
        // le kernel de traitement
        if( (stream_ptr == n_nucleotides) && (is_ended == true) ) {
            printf("FSM (Header => End)\n", stream_ptr);
            goto end;
        }
        if( stream_ptr == n_nucleotides )
        {
            // sinon on recharge un nouveau set d'informations
            n_nucleotides = reader->read( i_buffer, sizeof(char), ram_limit_in_MB);
            bytes_read   += n_nucleotides;
            is_ended      = (n_nucleotides != ram_limit_in_MB);
            stream_ptr    = 0;
            goto jump_header; // on
        }

        // On a trouvé le '\n', on retourne dans la partie calculatoire (nouvelle séquence => init)
        n_sequences += 1;

        if( stream_ptr > n_nucleotides ) {
            printf("(A) stream_ptr = %d | n_nucleotides = %d | is_ended = %d\n", stream_ptr, n_nucleotides, is_ended);
        }

        goto kernel_init;

    kernel_init:
#ifdef _debug_fsm_
        printf("FSM (kernel_init)\n");
#endif

        for(int x = 0; x < kmer_size - 1; x += 1)
        {
            //
            // on recharge les data si cela est necessaire
            //
            if( (stream_ptr == n_nucleotides) && (is_ended == false) )
            {
//                printf("(DD) Reload init\n");
                n_nucleotides = reader->read( i_buffer, sizeof(char), ram_limit_in_MB);
                bytes_read   += n_nucleotides;
                is_ended      = (n_nucleotides != ram_limit_in_MB);
                stream_ptr    = 0;
            }

            //
            // Encode les nucleotides du premier k-mer
            //
            const uint64_t nucleo  = i_buffer[stream_ptr];
            stream_ptr            += 1; // on positionne le ptr sur le next char (gestion des N)

            // Si '/n' ou 'N' on ne fait rien

            if( (nucleo == '\n') || (nucleo == '\r') )
            {
                x -= 1;     // On doit reculer le compteur d'une case car on ne prend pas en compte le nucléotide
                continue;   // actuel
            }else if( nucleo == 'N' ){
                goto kernel_init;
            }
            const uint64_t encoded = ((nucleo >> 1) & 0b11);
            current_mmer         <<= 2;
            current_mmer          |= encoded; // conversion ASCII => 2bits (Yoann)
            current_mmer          &= mask;
            cur_inv_mmer         >>= 2;
            cur_inv_mmer          |= ( (0x2 ^ encoded) << (2 * (kmer_size - 1))); // cf Yoann (forme cannonique)

            if( verbose_flag )
            {
                printf("%2d [%c] ", stream_ptr-1, nucleo);
                print_kmer(current_mmer, kmer_size); printf(" - ");
                print_kmer(cur_inv_mmer, kmer_size); printf("\n");
            }

            if( stream_ptr == n_nucleotides )
            {
                if(  is_ended == false ){
                    n_nucleotides = reader->read( i_buffer, sizeof(char), ram_limit_in_MB);
                    bytes_read   += n_nucleotides;
                    is_ended      = (n_nucleotides != ram_limit_in_MB);
                    stream_ptr    = 0;
                }else{
                    printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
                    exit( EXIT_FAILURE );
                }
            }

        }
        if( stream_ptr > n_nucleotides ) {
            printf("(0) stream_ptr = %d | n_nucleotides = %d | is_ended = %d\n", stream_ptr, n_nucleotides, is_ended);
        }

        goto kernel_loop;

    kernel_loop:
#ifdef _debug_fsm_
        printf("FSM (kernel_loop)\n");
#endif
        if( stream_ptr > n_nucleotides ) {
            printf("(1) stream_ptr = %d | n_nucleotides = %d | is_ended = %d\n", stream_ptr, n_nucleotides, is_ended);
        }

//        printf("position de debut de kmer = %lld\n", stream_ptr);
        while( true ) // On traite le reste des k-mers
        {
            //
            // Encode les nucleotides au fil de l'eau
            //

            const uint64_t nucleo  = i_buffer[stream_ptr];
            stream_ptr            += 1; // on positionne le ptr sur le next char (gestion des N)

            //
            // on recharge les data
            //
            if( (stream_ptr == n_nucleotides) && (is_ended == false) )
            {
//              printf("(DD) Reload kernel_loop\n");
                n_nucleotides = reader->read( i_buffer, sizeof(char), ram_limit_in_MB);
                bytes_read   += n_nucleotides;
                is_ended      = (n_nucleotides != ram_limit_in_MB);
                stream_ptr    = 0;
            }

            //
            // gestion des 'N' et des '>' et autres bizarreries
            //

            if( (nucleo == '\n') || (nucleo == '\r') )
            {
                continue;
            }

            const uint64_t encoded = ((nucleo >> 1) & 0b11); // conversion ASCII => 2bits (Yoann)
            current_mmer         <<= 2;                              // fonctionne pour les MAJ et les MIN
            current_mmer          |= encoded;
            current_mmer          &= mask;
            cur_inv_mmer         >>= 2;
            cur_inv_mmer          |= ((0x2 ^ encoded) << (2 * (kmer_size - 1))); // cf Yoann
            canonnic               = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;

            if( verbose_flag )
            {
                printf("%2d [%c] ", stream_ptr, nucleo);
                print_kmer(current_mmer, kmer_size); printf(" - ");
                print_kmer(cur_inv_mmer, kmer_size); printf(" ==> ");
                print_kmer(canonnic, kmer_size); printf("\n");
            }

            //
            // On peut faire le test apres car de tout facon, on va reinitialiser
            // les valeurs de current_mmer et cur_inv_mmer
            //
            if( nucleo == '>' )
                goto jump_header;
            else if( nucleo == 'N' )
            {
                n_non_valid += 1;
                goto kernel_init;
            }

            liste_mini[n_elements++] = canonnic;
            n_kmers                 += 1;

            if( stream_ptr > n_nucleotides ) {
                printf("(2) stream_ptr = %d | n_nucleotides = %d | is_ended = %d\n", stream_ptr, n_nucleotides, is_ended);
            }
            // On stocke sur disque
            if( n_elements == max_ou_ram )
            {
                const int64_t MB = n_kmers / 1024 / 1024;
                printf("- On flush ! (%lld M-kmers)\n", MB);

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
//              goto kernel_loop;   // on continue le traitement des données
            }

            if( (stream_ptr == 0) && (is_ended == true) )
                goto end;
        }

        if( stream_ptr > n_nucleotides )
        {
            printf("stream_ptr = %d | n_nucleotides = %d | is_ended = %d\n", stream_ptr, n_nucleotides, is_ended);
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit( EXIT_FAILURE );
        }


    end:
#ifdef _debug_fsm_
    printf("FSM (end)\n");
#endif
    printf("stream_ptr = %d | n_nucleotides = %d | is_ended = %d\n", stream_ptr, n_nucleotides, is_ended);
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
        for(int i = 0; i < liste_mini.size(); i += 1)
        {
            printf("%2d : ", i);
            print_kmer(liste_mini[i], kmer_size);
            printf(" - %6d\n", liste_cntr[i]);
        }
    }

    printf("Stats:\n");
    printf("  No. of k-mers below min. threshold :            0\n");
    printf("  No. of k-mers above max. threshold :            0\n");
    printf("  No. of unique k-mers               : %12zu\n",  liste_mini.size());
    printf("  No. of unique counted k-mers       : %12lld\n", liste_mini.size());
    printf("  Total no. of k-mers                : %12lld\n", n_kmers);
    printf("  Total no. of sequences             : %12lld\n", n_sequences);
    printf("  Total no. of super-k-mers          :            0\n");
    printf("  Total no. non valid nucleotides    : %12lld\n", n_non_valid);

    const int64_t kbytes_read =  bytes_read / 1024;
    const int64_t mbytes_read = kbytes_read / 1024;
    const float   gbytes_read = (float)mbytes_read / 1024.f;

    printf("IOs:\n");
    printf("  Total no. of  bytes reads          : %12lld\n",  bytes_read);
    printf("  Total no. of Kbytes reads          : %12lld\n", kbytes_read);
    printf("  Total no. of Mbytes reads          : %12lld\n", mbytes_read);
    printf("  Total no. of Gbytes reads          : %12.3f\n", gbytes_read);

    printf("Elapsed time %1.3f\n", start_gen.get_time_sec());

    if( file_save_debug )
        SaveMiniToTxtFile_v2(o_file + ".txt", liste_mini);

    if( file_save_output )
        SaveRawToFile(o_file, liste_mini);

    delete[] i_buffer;
    delete     reader;
}
