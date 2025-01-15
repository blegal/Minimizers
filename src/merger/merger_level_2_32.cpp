#include "merger_level_1.hpp"

void merge_level_2_32(std::string& ifile_1, std::string& ifile_2, std::string& o_file, const int level)
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
                    dest[ndst-1] |= colorDocA;
                }
                last_value = v1;
                counterA  += 2;
            } else {
                if (v1 != last_value){
                    dest[ndst++] = v2;
                    dest[ndst++] = colorDocB;
                }else{
                    dest[ndst-1] |= colorDocB;
                }
                last_value = v2;
                counterB  += 2;
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
        fwrite(in_2 + nElementsB, sizeof(uint64_t), nElementsB - counterB, fdst);
        do{
            nElementsB = fread(in_2, sizeof(uint64_t), _iBuff_, fin_2);
            ndst = 0;
            fwrite(in_2, sizeof(uint64_t), nElementsB, fdst);
        }while(nElementsB == _iBuff_);

    }else if (nElementsB == 0) {
        fwrite(in_1 + nElementsA, sizeof(uint64_t), nElementsA - counterA, fdst);
        do{
            nElementsA = fread(in_1, sizeof(uint64_t), _iBuff_, fin_1);
            fwrite(in_1, sizeof(uint64_t), nElementsA, fdst);
        }while(nElementsB == _iBuff_);
    }

    fclose( fin_1 );
    fclose( fin_2 );
    fclose( fdst  );

    delete [] in_1;
    delete [] in_2;
    delete [] dest;
}
