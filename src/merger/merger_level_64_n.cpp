#include "merger_level_1.hpp"

void merge_level_64_n(std::string& ifile_1, std::string& ifile_2, std::string& o_file, const int level)
{
    const int n_u64_per_min = level / 64; // En entrée de la fonction

    const int64_t _iBuff_ = (1 +     n_u64_per_min) * 1024;
    const int64_t _oBuff_ = (1 + 2 * n_u64_per_min) * 1024;

    uint64_t *in_1 = new uint64_t[_iBuff_];
    uint64_t *in_2 = new uint64_t[_iBuff_];
    uint64_t *dest = new uint64_t[_oBuff_];

    int64_t nElementsA = 0; // nombre d'éléments chargés en mémoire
    int64_t nElementsB = 0; // nombre d'éléments chargés en mémoire

    int64_t counterA = 0; // nombre de données lues dans le flux
    int64_t counterB = 0; // nombre de données lues dans le flux
    int64_t ndst     = 0; // nombre de données écrites dans le flux

    FILE *fin_1 = fopen(ifile_1.c_str(), "r");
    FILE *fin_2 = fopen(ifile_2.c_str(), "r");
    FILE *fdst  = fopen(o_file.c_str(),   "w");

    uint64_t last_value = 0xFFFFFFFFFFFFFFFF;
    while (true) {

        if (counterA == nElementsA) {
            nElementsA = fread(in_1, sizeof(uint64_t), _iBuff_, fin_1);
            if (nElementsA == 0) {
                break;
            }
            counterA = 0;

        }
        if (counterB == nElementsB) {
            nElementsB = fread(in_2, sizeof(uint64_t), _iBuff_, fin_2);
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
            fwrite(dest, sizeof(uint64_t), ndst, fdst);
            ndst = 0;
        }
    }

    //
    // Il faut absolument faire un check pour verifier qu'il n'y a pas de redondance dans le flux
    // qui n'est pas encore vide sinon on aura un doublon lors du flush !
    //
    if (nElementsA == 0) {
        const uint64_t v2 = in_2[counterB];
        const uint64_t colorDocB = in_2[counterB + 1] << level; // On decale de 2 bits au level 2, 4 au level 4, etc.
        if (v2 == last_value) {
            for(int c = 0; c < n_u64_per_min; c +=1)
                dest[ndst - 1 * n_u64_per_min + c] = in_2[counterB + 1 + c];
            counterB += (1 + n_u64_per_min);
        }
    }else if (nElementsB == 0) {
        const uint64_t v1        = in_1[counterA    ];
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
        fwrite(dest, sizeof(uint64_t), ndst, fdst);
        ndst = 0;
    }
oups en 32 aussi ?
    if (nElementsA == 0) {
        //
        // On insere les elements restant dans le buffer de dest.
        //
        for(int i = counterB; i < nElementsB; i += 1 + n_u64_per_min)
            in_2[i] = in_2[i] << level; // OK c verifié
        fwrite(in_2 + counterB, sizeof(uint64_t), nElementsB - counterB, fdst);

        //
        // On traite les donées restantes en passant par le buffer de dest.
        //
        do{
            nElementsB = fread(in_2, sizeof(uint64_t), _iBuff_, fin_2);
            for(int i = 1; i < nElementsB; i+= 1 + n_u64_per_min)
                in_2[i] = in_2[i] << level;
            if( nElementsB != 0 )
                fwrite(in_2, sizeof(uint64_t), nElementsB, fdst);
        }while(nElementsB == _iBuff_);

    }else if (nElementsB == 0) {
        //
        // On insere les elements restant dans le buffer de dest.
        //
        for(int i = counterA; i < nElementsA; i+= 2)
            in_1[i] = in_1[i];
        fwrite(in_1 + counterA, sizeof(uint64_t), nElementsA - counterA, fdst);

        //
        // On traite les donées restantes en passant par le buffer de dest.
        //
        do{
            nElementsA = fread(in_1, sizeof(uint64_t), _iBuff_, fin_1);
            for(int i = 1; i < nElementsA; i+= 2) in_1[i] = in_1[i] << level;
            if( nElementsA != 0 )
                fwrite(in_1, sizeof(uint64_t), nElementsA, fdst);
        }while(nElementsA == _iBuff_);
    }

    fclose( fin_1 );
    fclose( fin_2 );
    fclose( fdst  );

    delete [] in_1;
    delete [] in_2;
    delete [] dest;
}
