#include "minimizer.hpp"
#include "deduplication.hpp"

#include "../front/fastx/read_fastx_file.hpp"
#include "../front/fastx_bz2/read_fastx_bz2_file.hpp"

#include "../front/count_file_lines.hpp"

#include "../kmer/bfc_hash64.hpp"

#include "../hash//CustomMurmurHash3.hpp"

#include "../back/txt/SaveMiniToTxtFile.hpp"
#include "../back/raw/SaveMiniToRawFile.hpp"

#include "../sorting/std_2cores/std_2cores.hpp"
#include "../sorting/std_4cores/std_4cores.hpp"
#include "../sorting/crumsort_2cores/crumsort_2cores.hpp"

#define MEM_UNIT 64ULL

inline uint64_t mask_right(const uint64_t numbits)
{
    uint64_t mask = -(numbits >= MEM_UNIT) | ((1ULL << numbits) - 1ULL);
    return mask;
}

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
    const int max_in_ram = 1024 * 1024* ram_limit_in_MB;

    /*
     * Reading the K value from the first file line
     */
    const int kmer = 31;
    const int mmer = 19;
    const int z = kmer - mmer;

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
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "fastx")
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

    std::tuple<int, bool> mTuple = reader->next_sequence(seq_value, 4096);
    printf("first = %s\n", seq_value);

    while( true )
    {

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
            cur_inv_mmer |= (encoded << (2 * (mmer - 1)));

            cnt          += 1;
        }


        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // On traite le premier k-mer. Pour cela on calcule ses differents s-mer afin d'en extraire la
        // valeur minimum.
        //
        uint64_t minv = UINT64_MAX;
        for(int m_pos = 0; m_pos <= z; m_pos += 1)
        {
            // On calcule les m-mers
            const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11); // conversion ASCII => 2bits (Yoann)
            current_mmer <<= 2;
            current_mmer |= encoded;
            current_mmer &= mask;
            cur_inv_mmer >>= 2;
            cur_inv_mmer |= (encoded << (2 * (mmer - 1)));          // conversion ASCII => 2bits (Yoann)

            const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;

#if 0
            //
            //
            uint64_t tab[2];
            CustomMurmurHash3_x64_128<8> ( &canon, 42, tab );
            const uint64_t s_hash = tab[0];
#else
            //
            //
            const uint64_t s_hash = bfc_hash_64(canon, mask);
#endif
            //
            //
            buffer[m_pos]         = s_hash; // on memorise le hash du mmer
            minv                  = (s_hash < minv) ? s_hash : minv;
            cnt                  += 1; // on avance dans la sequence de nucleotides
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //
        // On memorise la valeur du minimiser du premier kmer de la sequence dans le buffer de sortie
        //
        if( n_minizer == 0 ){
            liste_mini[n_minizer++] = minv;
        }else if( liste_mini[n_minizer-1] != minv ){
            liste_mini[n_minizer++] = minv;
        }else{
            n_skipper += 1;
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
        int nELements     = std::get<0>(mTuple) - mmer + 1;
        int isSeqFinished = false;

        //
        // Les sequences de nucleotides peuvent être réparties sur plusieurs lignes
        //
        while( isSeqFinished == false )
        {
            for(int k_pos = kmerStartIdx; k_pos < nELements; k_pos += 1) // On traite le reste des k-mers
            {
                const uint64_t encoded = ((ptr_kmer[cnt] >> 1) & 0b11); // conversion ASCII => 2bits (Yoann)
                current_mmer <<= 2;                                     // fonctionne pour les MAJ et les MIN
                current_mmer |= encoded;
                current_mmer &= mask;
                cur_inv_mmer >>= 2;
                cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); // cf Yoann

                const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;
                const uint64_t s_hash = bfc_hash_64(canon, mask);
                minv                  = (s_hash < minv) ? s_hash : minv;

//                if( minv == buffer[0] )
//                {
                    minv = s_hash;
                    for(int p = 0; p < z + 1; p += 1) {
                        const uint64_t value = buffer[p+1];
                        minv = (minv < value) ? minv : value;
                        buffer[p] = value;
                    }
                    buffer[z] = s_hash; // on memorise le hash du dernier m-mer
//                }else{
//                    for(int p = 0; p < z+1; p += 1) {
//                        buffer[p] = buffer[p+1];
//                    }
//                    buffer[z] = s_hash; // on memorise le hash du dernier m-mer
//                }

                ////////////////////////////////////////////////////////////////////////////////////

                if( liste_mini[n_minizer-1] != minv )
                {
                    liste_mini[n_minizer++] = minv;
                }else{
                    n_skipper += 1;
                }

                kmer_cnt += 1; // on a traité un kmer de plus
                cnt      += 1; // on avance le pointeur dans le flux
            }

            //
            // On lance le chargement de la ligne suivante
            //
            mTuple        = reader->next_sequence(seq_value, 4096);
            kmerStartIdx  = 0;
            nELements     = std::get<0>(mTuple);
            isSeqFinished = std::get<1>(mTuple);
            cnt           = 0;

            if( isSeqFinished == false )
                printf("+ next = %s\n", seq_value);

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

    }

    // On vient de finir le traitement d'une ligne. On peut en profiter pour purger le
    // buffer de sortie sur le disque dur apres l'avoir purgé des doublons et l'avoir
    // trié...

    liste_mini.resize(n_minizer);

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

    VectorDeduplication( liste_mini );

    if( file_save_debug ){
        SaveMiniToTxtFile_v2(o_file + ".txt", liste_mini);
    }

    if( file_save_output ){
        SaveMiniToFileRAW(o_file, liste_mini);
    }

    delete reader;

}
