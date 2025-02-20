#include "merger_level_1.hpp"

void merge_level_2_32(
        const std::string& ifile_1,
        const std::string& ifile_2,
        const std::string& o_file,
        const int level)
{

    const int64_t _iBuff_ = 64 * 1024;
    const int64_t _oBuff_ = _iBuff_; // 2 fois plus grand car on insere les couleurs

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
            const uint64_t v1        = in_1[counterA    ];
            const uint64_t colorDocA = in_1[counterA + 1];
            const uint64_t v2        = in_2[counterB    ];
            const uint64_t colorDocB = in_2[counterB + 1] << level; // On decale de 2 bits au level 2, 4 au level 4, etc.

            if (v1 < v2) {
                if (v1 != last_value){
                    dest[ndst++] = v1;
                    dest[ndst++] = colorDocA;
                }else{
//                    printf("merge colorDoc1 (%16.16llX / %16.16llX)\n", dest[ndst-1], colorDocA);
                    if( ndst == 0 )
                        exit(EXIT_FAILURE);
                    dest[ndst-1] |= colorDocA;
//                    printf("                (%16.16llX)\n", dest[ndst-1]);
                }
                last_value = v1;
                counterA  += 2;
            } else {
                if (v2 != last_value){
                    dest[ndst++] = v2;
                    dest[ndst++] = colorDocB;
                }else{
//                    printf("merge colorDocB (%16.16llX / %16.16llX)\n", dest[ndst-1], colorDocB);
                    dest[ndst-1] |= colorDocB;
//                    printf("                (%16.16llX)\n", dest[ndst-1]);
                }
                last_value = v2;
                counterB  += 2;
            }
        }

        //
        //
        //
        if (ndst == _oBuff_) {
            fwrite(dest, sizeof(uint64_t), ndst - 2, fdst);
            dest[0] = dest[_oBuff_ - 2]; // we should keep the last value in case the next
            dest[1] = dest[_oBuff_ - 1]; // processed value is the same (should update the color)
            ndst = 2;
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
            dest[ndst - 1] |= colorDocB;
            counterB += 2;
        }
    }else if (nElementsB == 0) {
        const uint64_t v1        = in_1[counterA    ];
        const uint64_t colorDocA = in_1[counterA + 1];
        if (v1 == last_value){
            dest[ndst-1] |= colorDocA;
            counterA  += 2;
        }
    }

    //
    // On realise le flush du buffer de sortie, cela nous simplifier la vie par la suite !
    //
    if (ndst != 0) {
//        printf("flush dest buffer (Z-1) : (%16.16llX, %16.16llX)\n", dest[ndst-2], dest[ndst-1]);
//        printf("                          (%16.16llX)\n", dest[ndst-1]);
        fwrite(dest, sizeof(uint64_t), ndst, fdst);
        ndst = 0;
    }

    if (nElementsA == 0) {
//        printf("flush B : %lld %lld\n", counterB, nElementsB);
        for(int i = counterB + 1; i < nElementsB; i+= 2) in_2[i] = in_2[i] << level;
        fwrite(in_2 + counterB, sizeof(uint64_t), nElementsB - counterB, fdst);
        do{
            nElementsB = fread(in_2, sizeof(uint64_t), _iBuff_, fin_2);
            for(int i = 1; i < nElementsB; i+= 2) in_2[i] = in_2[i] << level;
//            printf(" - flush B : %lld\n", nElementsB);
            if( nElementsB != 0 )
                fwrite(in_2, sizeof(uint64_t), nElementsB, fdst);
        }while(nElementsB == _iBuff_);

    }else if (nElementsB == 0) {
//        printf("flush A : %lld %lld\n", counterA, nElementsA);
        for(int i = counterA + 1; i < nElementsA; i+= 2) in_1[i] = in_1[i];
        fwrite(in_1 + counterA, sizeof(uint64_t), nElementsA - counterA, fdst);
        do{
            nElementsA = fread(in_1, sizeof(uint64_t), _iBuff_, fin_1);
            for(int i = 1; i < nElementsA; i+= 2) in_1[i] = in_1[i];
//            printf(" - flush A : %lld\n", nElementsA);
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
