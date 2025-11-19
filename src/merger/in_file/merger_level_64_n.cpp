#include "merger_level_1.hpp"
#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"

template< int level >
void merge_level_64_n_t(
        const std::string& ifile_1,
        const std::string& ifile_2,
        const std::string& o_file)
{
    const int n_u64_per_min = (level+63) / 64; // En entrée de la fonction

    const int64_t _iBuff_ = (1 +     n_u64_per_min) * 1024;
    const int64_t _oBuff_ = (1 + 2 * n_u64_per_min) * 1024; // la sortie contient 2 x couleurs

    uint64_t *in_1 = new uint64_t[_iBuff_];
    uint64_t *in_2 = new uint64_t[_iBuff_];
    uint64_t *dest = new uint64_t[_oBuff_];

    int64_t nElementsA = 0; // nombre d'éléments chargés en mémoire
    int64_t nElementsB = 0; // nombre d'éléments chargés en mémoire

    int64_t counterA = 0; // nombre de données lues dans le flux
    int64_t counterB = 0; // nombre de données lues dans le flux
    int64_t ndst     = 0; // nombre de données écrites dans le flux

    stream_reader* fin_1 = stream_reader_library::allocate( ifile_1 );
    stream_reader* fin_2 = stream_reader_library::allocate( ifile_2 );
    stream_writer* fdst  = stream_writer_library::allocate( o_file );

    uint64_t last_value = 0xFFFFFFFFFFFFFFFF;
    while (true) {

        if (counterA == nElementsA) {
            nElementsA = fin_1->read(in_1, sizeof(uint64_t), _iBuff_);
            if (nElementsA == 0) {
                break;
            }
            counterA = 0;

        }
        if (counterB == nElementsB) {
            nElementsB = fin_2->read(in_2, sizeof(uint64_t), _iBuff_);
            if (nElementsB == 0) {
                break;
            };
            counterB = 0;
        }

        while ((counterA != nElementsA) && (counterB != nElementsB) && ndst != (_oBuff_)) {
            const uint64_t v1        = in_1[counterA];
            const uint64_t v2        = in_2[counterB];

            if (v1 < v2) {
                if (v1 != last_value){
                    // on ajoute notre minimizer dans la sortie
                    dest[ndst++] = v1;
                    // on insere notre couleur (A) dans la sortie
                    for(int c = 0; c < n_u64_per_min; c +=1)
                        dest[ndst++] = in_1[counterA + 1 + c];
                    // on ajoute des zero pour les autres culeurs
                    for(int c = 0; c < n_u64_per_min; c +=1)
                        dest[ndst++] = 0;
                }else{
                    if( (ndst == 0) || (ndst%(2 * n_u64_per_min + 1) != 0) )
                    {
                        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
                        printf("(EE) ndst = %ld\n", ndst);
                        printf("(EE) n_u64_per_min = %d\n", n_u64_per_min);
                        exit( EXIT_FAILURE );
                    }

                    // on recopie notre couleur (A) dans la sortie
                    for(int c = 0; c < n_u64_per_min; c +=1)
                        dest[ndst - 2 * n_u64_per_min + c] = in_1[counterA + 1 + c];
                }
                last_value = v1;
                counterA  += (1 + n_u64_per_min);
            } else {
                if (v2 != last_value){
                    dest[ndst++] = v2;
                    for(int c = 0; c < n_u64_per_min; c +=1)
                        dest[ndst++] = 0;
                    for(int c = 0; c < n_u64_per_min; c +=1)
                        dest[ndst++] = in_2[counterB + 1 + c];
                }else{
                    if( (ndst == 0) || (ndst%(2 * n_u64_per_min + 1) != 0) )
                    {
                        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
                        printf("(EE) ndst          = %ld\n", ndst);
                        printf("(EE) n_u64_per_min = %d\n", n_u64_per_min);
                        exit( EXIT_FAILURE );
                    }
                    for(int c = 0; c < n_u64_per_min; c +=1)
                        dest[ndst - 1 * n_u64_per_min + c] = in_2[counterB + 1 + c];
                }
                last_value = v2;
                counterB  += (1 + n_u64_per_min);
            }
        }

        //
        //
        //
        if (ndst == _oBuff_) {
            fdst->write(dest, sizeof(uint64_t), ndst - 1 - 2 * n_u64_per_min); // we should keep one element
            //
            ndst = 0;
            dest[ndst++] = dest[_oBuff_ - 1 - 2 * n_u64_per_min]; // we should keep the last value in case the next
            for(int c = 0; c < (2 * n_u64_per_min); c +=1)
                dest[ndst++] = dest[_oBuff_ - 2 * n_u64_per_min + c];
        }
    }

    //
    // Il faut absolument faire un check pour verifier qu'il n'y a pas de redondance dans le flux
    // qui n'est pas encore vide sinon on aura un doublon lors du flush !
    //
    if (nElementsA == 0) {
        const uint64_t v2 = in_2[counterB];
//        printf("Should we merge B : %16.16llX %16.16llX\n", v2, last_value);
        if (v2 == last_value) {
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst - 1 * n_u64_per_min + c] = in_2[counterB + 1 + c];
            counterB += (1 + n_u64_per_min);
        }

    }else if (nElementsB == 0) {
        const uint64_t v1        = in_1[counterA];
//        printf("Should we merge A : %16.16llX %16.16llX\n", v1, last_value);
        if (v1 == last_value){
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst - 2 * n_u64_per_min + c] = in_1[counterA + 1 + c];
            counterA  += (1 + n_u64_per_min);
        }
    }

    //
    // On realise le flush du buffer de sortie, cela nous simplifier la vie par la suite !
    //
    if (ndst != 0) {
        fdst->write(dest, sizeof(uint64_t), ndst);
        ndst = 0;
    }

    if (nElementsA == 0) {
        //
        // On insere les elements restant dans le buffer de dest.
        //
        for(int i = counterB; i < nElementsB; i += 1 + n_u64_per_min)
        {
//            printf("B adds: %16.16llX\n", in_2[i]);
            dest[ndst++] = in_2[i];
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst++] = 0;
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst++] = in_2[i + 1 + c];
        }
        fdst->write(dest, sizeof(uint64_t), ndst);
        ndst = 0;
        //
        // On traite les donées restantes en passant par le buffer de dest.
        //
        do{
            nElementsB = fin_2->read(in_2, sizeof(uint64_t), _iBuff_);

            for(int i = 0; i < nElementsB; i += 1 + n_u64_per_min) {
//                printf("+B adds: %16.16llX\n", in_2[i]);
                dest[ndst++] = in_2[i];
                for (int c = 0; c < n_u64_per_min; c += 1)
                    dest[ndst++] = 0;
                for (int c = 0; c < n_u64_per_min; c += 1)
                    dest[ndst++] = in_2[i + 1 + c];
            }

            if( nElementsB != 0 ){
                fdst->write(dest, sizeof(uint64_t), ndst);
                ndst = 0;
            }
        }while(nElementsB == _iBuff_);

    }else if (nElementsB == 0) {
        //
        // On insere les elements restant dans le buffer de dest.
        //
        for(int i = counterA; i < nElementsA; i += 1 + n_u64_per_min)
        {
            // on ajoute notre minimizer dans la sortie
//            printf("A adds: %16.16llX\n", in_1[i]);
            dest[ndst++] = in_1[i];
            // on insere notre couleur (A) dans la sortie
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst++] = in_1[i + 1 + c];
            // on ajoute des zero pour les autres culeurs
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst++] = 0;
        }
        fdst->write(dest, sizeof(uint64_t), ndst);
        ndst = 0;

        //
        // On traite les donées restantes en passant par le buffer de dest.
        //
        do{
            nElementsA = fin_1->read(in_1, sizeof(uint64_t), _iBuff_);

            for(int i = 0; i < nElementsA; i += 1 + n_u64_per_min) {
//                printf("+A adds: %16.16llX\n", in_1[i]);
                dest[ndst++] = in_1[i];
                for (int c = 0; c < n_u64_per_min; c += 1)
                    dest[ndst++] = in_1[i + 1 + c];
                for (int c = 0; c < n_u64_per_min; c += 1)
                    dest[ndst++] = 0;
            }

            if( nElementsA != 0 )
            {
                fdst->write(dest, sizeof(uint64_t), ndst);
                ndst = 0;
            }
        }while(nElementsA == _iBuff_);
    }

    delete fin_1;
    delete fin_2;
    delete fdst;

    delete [] in_1;
    delete [] in_2;
    delete [] dest;
}

void merge_level_64_n(
        const std::string& ifile_1,
        const std::string& ifile_2,
        const std::string& o_file,
        const int level)
{
    if( level == 64 )
        merge_level_64_n_t<64>(ifile_1, ifile_2, o_file);
    else if( level == 128 )
        merge_level_64_n_t<128>(ifile_1, ifile_2, o_file);
    else if( level == 256 )
        merge_level_64_n_t<256>(ifile_1, ifile_2, o_file);
    else if( level == 512 )
        merge_level_64_n_t<512>(ifile_1, ifile_2, o_file);
    else if( level == 1024 )
        merge_level_64_n_t<1024>(ifile_1, ifile_2, o_file);
    else if( level == 2048 )
        merge_level_64_n_t<2048>(ifile_1, ifile_2, o_file);
    else if( level == 4096 )
        merge_level_64_n_t<4096>(ifile_1, ifile_2, o_file);
    else if( level == 8192 )
        merge_level_64_n_t<8192>(ifile_1, ifile_2, o_file);
    else if( level == 16384 )
        merge_level_64_n_t<16384>(ifile_1, ifile_2, o_file);
    else if( level == 32768 )
        merge_level_64_n_t<32768>(ifile_1, ifile_2, o_file);
    else if( level == 65536 )
        merge_level_64_n_t<65536>(ifile_1, ifile_2, o_file);
    else{
        printf("%s %d\n", __FILE__, __LINE__);
        printf("NOT supported yet (%d)\n", level);
        exit( EXIT_FAILURE );
    }

}
