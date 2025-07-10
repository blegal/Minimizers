#include "ram_merger_level_1.hpp"

template< int level >
void ram_merge_level_64_n_t(
        CFileBuffer* ifile_1,
        CFileBuffer* ifile_2,
        CFileBuffer* o_file)
{
    const int n_u64_per_min = level / 64; // En entrée de la fonction

    //
    // On calcule la taille maximum que peux avoir le fichier en
    // en sortie de l'étape de fusion des données sachant que les
    // données d'entrée n'ont pas de couleur
    //

    const int64_t nElementsA    = ifile_1->uint64_in_ram();
    const int64_t nElementsB    = ifile_2->uint64_in_ram();
    const int64_t nElements_max = (1 + 2 * n_u64_per_min) * (nElementsA / (1 + n_u64_per_min) + nElementsB / (1 + n_u64_per_min));

    //
    // On redimentionne le buffer alloué en mémoire
    //

    o_file->resize_ram_size( nElements_max );

    //
    // On initialise nos compteurs de données et nos couleurs
    //

    int64_t counterA = 0; // nombre de données lues dans le flux
    int64_t counterB = 0; // nombre de données lues dans le flux
    int64_t ndst     = 0; // nombre de données écrites dans le flux

    //
    // On récupère les pointeurs sur nos espaces mémoire
    //

    const uint64_t* in_1 = ifile_1->data.data();
    const uint64_t* in_2 = ifile_2->data.data();
          uint64_t* dest =  o_file->data.data();

    uint64_t last_value = 0xFFFFFFFFFFFFFFFF;

    while ((counterA != nElementsA) && (counterB != nElementsB))
    {
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
                    printf("(EE) ndst          = %ld\n", ndst);
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
    // Il faut absolument faire un check pour verifier qu'il n'y a pas de redondance dans le flux
    // qui n'est pas encore vide sinon on aura un doublon lors du flush !
    //
    if (counterA == nElementsA) {
        const uint64_t v2 = in_2[counterB];
        if (v2 == last_value) {
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst - 1 * n_u64_per_min + c] = in_2[counterB + 1 + c];
            counterB += (1 + n_u64_per_min);
        }

        for(int i = counterB; i < nElementsB; i += 1 + n_u64_per_min)
        {
//            printf("B adds: %16.16llX\n", in_2[i]);
            dest[ndst++] = in_2[i];
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst++] = 0;
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst++] = in_2[i + 1 + c];
        }


    }else if (counterB == nElementsB) {
        const uint64_t v1        = in_1[counterA];
        if (v1 == last_value){
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst - 2 * n_u64_per_min + c] = in_1[counterA + 1 + c];
            counterA  += (1 + n_u64_per_min);
        }

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

    }

    //
    // On redimentionne le CFileBuffer à la bonne capacité
    //
    o_file->resize_ram_size( ndst );
}

void ram_merge_level_64_n(
        CFileBuffer* ifile_1,
        CFileBuffer* ifile_2,
        CFileBuffer* o_file,
        const int level)
{
    if( level == 64 )
        ram_merge_level_64_n_t<64>(ifile_1, ifile_2, o_file);
    else if( level == 128 )
        ram_merge_level_64_n_t<128>(ifile_1, ifile_2, o_file);
    else if( level == 256 )
        ram_merge_level_64_n_t<256>(ifile_1, ifile_2, o_file);
    else if( level == 512 )
        ram_merge_level_64_n_t<512>(ifile_1, ifile_2, o_file);
    else if( level == 1024 )
        ram_merge_level_64_n_t<1024>(ifile_1, ifile_2, o_file);
    else if( level == 2048 )
        ram_merge_level_64_n_t<2048>(ifile_1, ifile_2, o_file);
    else if( level == 4096 )
        ram_merge_level_64_n_t<4096>(ifile_1, ifile_2, o_file);
    else if( level == 8192 )
        ram_merge_level_64_n_t<8192>(ifile_1, ifile_2, o_file);
    else if( level == 16384 )
        ram_merge_level_64_n_t<16384>(ifile_1, ifile_2, o_file);
    else if( level == 32768 )
        ram_merge_level_64_n_t<32768>(ifile_1, ifile_2, o_file);
    else if( level == 65536 )
        ram_merge_level_64_n_t<65536>(ifile_1, ifile_2, o_file);
    else{
        printf("%s %d\n", __FILE__, __LINE__);
        printf("NOT supported yet (%d)\n", level);
        exit( EXIT_FAILURE );
    }
}
