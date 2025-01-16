#include "merger_level_1.hpp"

void merge_level_1(
        const std::string& ifile_1,
        const std::string& ifile_2,
        const std::string& o_file)
{

    const int64_t _iBuff_ = 64 * 1024;
    const int64_t _oBuff_ = 2 * _iBuff_; // 2 fois plus grand car on insere les couleurs

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

    uint64_t colorDocA = 0x0000000000000001;
    uint64_t colorDocB = 0x0000000000000002;

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
            const uint64_t v1 = in_1[counterA];
            const uint64_t v2 = in_2[counterB];

            if (v1 < v2) {
                if (v1 != last_value){
                    dest[ndst++] = v1;
                    dest[ndst++] = colorDocA;
//                    printf("un (%3llu, %3llu)...\n", v1, v2);
                }else{
                    dest[ndst-1] |= colorDocA;
//                    printf("ici %3lld (%3llu, %3llu)...\n", last_value, v1, v2);
                }
                last_value = v1;
                counterA  += 1;
            } else {
                if (v1 != last_value){
                    dest[ndst++] = v2;
                    dest[ndst++] = colorDocB;
//                    printf("deux (%3llu, %3llu)...\n", v1, v2);
                }else{
                    dest[ndst-1] |= colorDocB;
//                    printf("la %3lld (%3llu, %3llu)...\n", last_value, v1, v2);
                }
                last_value = v2;
                counterB  += 1;
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

    if (ndst != 0) {
        fwrite(dest, sizeof(uint64_t), ndst, fdst);
        ndst = 0;
    }

    if (nElementsA == 0) {
        ndst = 0;
        for(int i = counterB; i < nElementsB; i += 1){ dest[ndst++] = in_2[i]; dest[ndst++] = colorDocB; }
        fwrite(dest, sizeof(uint64_t), ndst, fdst);
        do{
            nElementsB = fread(in_2, sizeof(uint64_t), _iBuff_, fin_2);
            ndst = 0;
            for( int i = 0; i < nElementsB; i += 1){ dest[ndst++] = in_2[i]; dest[ndst++] = colorDocB; }
            fwrite(dest, sizeof(uint64_t), ndst, fdst);
        }while(nElementsB == _iBuff_);

    }else if (nElementsB == 0) {
        ndst = 0;
        for(int i = counterA; i < nElementsA; i += 1){  dest[ndst++] = in_1[i]; dest[ndst++] = colorDocA; }
        fwrite(dest, sizeof(uint64_t), ndst, fdst);
        do{
            nElementsA = fread(in_1, sizeof(uint64_t), _iBuff_, fin_1);
            for( int i = 0; i < nElementsA; i += 1){  dest[ndst++] = in_1[i]; dest[ndst++] = colorDocA; }
            fwrite(dest, sizeof(uint64_t), ndst, fdst);
        }while(nElementsB == _iBuff_);
    }

    fclose( fin_1 );
    fclose( fin_2 );
    fclose( fdst  );

    delete [] in_1;
    delete [] in_2;
    delete [] dest;
}
