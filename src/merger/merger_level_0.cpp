#include "merger_level_0.hpp"

void merge_level_0(
        const std::string& ifile_1,
        const std::string& ifile_2,
        const std::string& o_file)
{
//    std::cout << "merging(" << ifile_1 << ", " << ifile_2 << ") => " << o_file << std::endl;

    const int64_t _iBuff_ = 64 * 1024;
    const int64_t _oBuff_ = _iBuff_;

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
//                printf("End A (nElementsA = %lld, nElementsB = %lld)\n", nElementsA, nElementsB);
                break;
            }
            counterA = 0;
//            printf("Read A\n");
        }
        if (counterB == nElementsB) {
            nElementsB = fread(in_2, sizeof(uint64_t), _iBuff_, fin_2);
            if (nElementsB == 0) {
//                printf("End B (nElementsA = %lld, nElementsB = %lld)\n", nElementsA, nElementsB);
                break;
            };
            counterB = 0;
//            printf("Read B\n");
        }

        while ((counterA != nElementsA) && (counterB != nElementsB) && ndst != (_oBuff_)) {
            const uint64_t v1 = in_1[counterA];
            const uint64_t v2 = in_2[counterB];
//            const uint64_t ll = last_value;
            if (v1 < v2) {
                if (v1 != last_value)
                    dest[ndst++] = v1;
                last_value = v1;
                counterA      += 1;
            } else {
                if (v1 != last_value)
                    dest[ndst++] = v2;
                last_value = v2;
                counterB      += 1;
            }
//            printf("last: 0x%16.16llX | v1: 0x%16.16llX | v2: 0x%16.16llX | cnt1: %llu | cnt2: %llu | dst: %llu |\n", ll, v1, v2, counterA, counterB, ndst);
        }

        //
        //
        //
        if (ndst == _oBuff_) {
            fwrite(dest, sizeof(uint64_t), ndst, fdst);
            ndst = 0;
//            printf("flush\n");
        }
    }

//    printf("End X (nElementsA = %lld, nElementsB = %lld)\n", nElementsA, nElementsB);

    if (ndst != 0) {
        fwrite(dest, sizeof(uint64_t), ndst, fdst);
        ndst = 0;
//        printf("flush (end)\n");
    }

//    printf("Ending\n");
    if (nElementsA == 0) {
//        printf("flushing B (%lld)\n", nElementsA);
        fwrite(in_2 + counterB, sizeof(uint64_t), nElementsB - counterB, fdst);
        do{
            nElementsB = fread(in_2, sizeof(uint64_t), _iBuff_, fin_2);
            fwrite(in_2, sizeof(uint64_t), nElementsB, fdst);
        }while(nElementsB == _iBuff_);

    }else if (nElementsB == 0) {
//        printf("flushing A (%lld)\n", nElementsB);
        fwrite(in_1 + counterA, sizeof(uint64_t), nElementsA - counterA, fdst);
        do{
            nElementsA = fread(in_1, sizeof(uint64_t), _iBuff_, fin_1);
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
