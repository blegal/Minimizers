#include "merger_level_0.hpp"
#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"

void merge_level_0(
        const std::string& ifile_1,
        const std::string& ifile_2,
        const std::string& o_file)
{
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

    stream_reader* fin_1 = stream_reader_library::allocate( ifile_1 );
    stream_reader* fin_2 = stream_reader_library::allocate( ifile_2 );
    stream_writer* fdst  = stream_writer_library::allocate( o_file );
  //FILE *fin_1 = fopen(ifile_1.c_str(), "r");
  //FILE *fin_2 = fopen(ifile_2.c_str(), "r");
  //FILE *fdst  = fopen(o_file.c_str(),  "w");

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
            const uint64_t v1 = in_1[counterA];
            const uint64_t v2 = in_2[counterB];
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
        }

        //
        //
        //
        if (ndst == _oBuff_) {
            fdst->write(dest, sizeof(uint64_t), ndst);
            ndst = 0;
        }
    }

    if (ndst != 0) {
        fdst->write(dest, sizeof(uint64_t), ndst);
        ndst = 0;
    }

    if (nElementsA == 0) {
        fdst->write(in_2 + counterB, sizeof(uint64_t), nElementsB - counterB);
        do{
            nElementsB = fin_2->read(in_2, sizeof(uint64_t), _iBuff_);
            fdst->write(in_2, sizeof(uint64_t), nElementsB);
        }while(nElementsB == _iBuff_);

    }else if (nElementsB == 0) {
        fdst->write(in_1 + counterA, sizeof(uint64_t), nElementsA - counterA);
        do{
            nElementsA = fin_1->read(in_1, sizeof(uint64_t), _iBuff_);
            fdst->write(in_1, sizeof(uint64_t), nElementsA);
        }while(nElementsA == _iBuff_);
    }

    delete fin_1;
    delete fin_2;
    delete fdst;

    delete [] in_1;
    delete [] in_2;
    delete [] dest;
}
