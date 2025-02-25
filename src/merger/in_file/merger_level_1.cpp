#include "merger_level_1.hpp"
#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"

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

    stream_reader* fin_1 = stream_reader_library::allocate( ifile_1 );
    stream_reader* fin_2 = stream_reader_library::allocate( ifile_2 );
    stream_writer* fdst  = stream_writer_library::allocate( o_file );

    uint64_t colorDocA = 0x0000000000000001;
    uint64_t colorDocB = 0x0000000000000002;

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
                if (v1 != last_value){
                    dest[ndst++] = v1;
                    dest[ndst++] = colorDocA;
                }else{
                    dest[ndst-1] |= colorDocA;
                }
                last_value = v1;
                counterA  += 1;
            } else {
                if (v2 != last_value){
                    dest[ndst++] = v2;
                    dest[ndst++] = colorDocB;
                }else{
                    dest[ndst-1] |= colorDocB;
                }
                last_value = v2;
                counterB  += 1;
            }
        }

        //
        //
        //
        if (ndst == _oBuff_) {
            fdst->write(dest, sizeof(uint64_t), ndst - 2);
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
        if (v2 == last_value) {
            dest[ndst - 1] |= colorDocB;
            counterB += 1;
        }
    }else if (nElementsB == 0) {
        const uint64_t v1 = in_1[counterA];
        if (v1 == last_value){
            dest[ndst-1] |= colorDocA;
            counterA  += 1;
        }
    }


    //
    // Il faut absolument faire un check pour verifier qu'il n'y a pas de redondance dans le flux
    // qui n'est pas encore vide sinon on aura un doublon lors du flush !
    //
    if (nElementsA == 0) {
        const uint64_t v2 = in_2[counterB];
        if (v2 == last_value) {
            dest[ndst - 1] |= colorDocB;
            counterB += 1;
        }
    }else if (nElementsB == 0) {
        const uint64_t v1 = in_1[counterA];
        if (v1 == last_value){
            dest[ndst-1] |= colorDocA;
            counterA  += 1;
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
        ndst = 0;
        for(int i = counterB; i < nElementsB; i += 1){
            dest[ndst++] = in_2[i];
            dest[ndst++] = colorDocB;
        }
        fdst->write(dest, sizeof(uint64_t), ndst);
        do{
            ndst       = 0;
            nElementsB = fin_2->read(in_2, sizeof(uint64_t), _iBuff_);
            for(int i = 0; i < nElementsB; i += 1){ dest[ndst++] = in_2[i]; dest[ndst++] = colorDocB; }
            fdst->write(dest, sizeof(uint64_t), ndst);
        }while(nElementsB == _iBuff_);

    }else if (nElementsB == 0) {
        ndst = 0;
        for(int i = counterA; i < nElementsA; i += 1){
            dest[ndst++] = in_1[i];
            dest[ndst++] = colorDocA;
        }
        fdst->write(dest, sizeof(uint64_t), ndst);
        do{
            ndst       = 0;
            nElementsA = fin_1->read(in_1, sizeof(uint64_t), _iBuff_);
            for(int i = 0; i < nElementsA; i += 1){  dest[ndst++] = in_1[i]; dest[ndst++] = colorDocA; }
            fdst->write(dest, sizeof(uint64_t), ndst);
        }while(nElementsA == _iBuff_);
    }

    delete fin_1;
    delete fin_2;
    delete fdst;

    delete [] in_1;
    delete [] in_2;
    delete [] dest;
}
