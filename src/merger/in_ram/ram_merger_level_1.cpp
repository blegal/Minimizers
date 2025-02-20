#include "ram_merger_level_1.hpp"

void ram_merge_level_1(
        CFileBuffer* ifile_1,
        CFileBuffer* ifile_2,
        CFileBuffer* o_file)
{
    //
    // On calcule la taille maximum que peux avoir le fichier en
    // en sortie de l'étape de fusion des données sachant que les
    // données d'entrée n'ont pas de couleur
    //

    const int64_t nElementsA    = ifile_1->uint64_in_ram();
    const int64_t nElementsB    = ifile_2->uint64_in_ram();
    const int64_t nElements_max = 2 * (nElementsA + nElementsB);

    //
    // On redimentionne le buffer alloué en mémoire
    //

    o_file->resize_ram_size( nElements_max );

    //
    // On initialise nos compteurs de données et nos couleurs
    //

    int64_t counterA = 0; // nombre de données lues dans le flux A
    int64_t counterB = 0; // nombre de données lues dans le flux B
    int64_t ndst     = 0; // nombre de données écrites dans le flux

    uint64_t colorDocA = 0x0000000000000001;
    uint64_t colorDocB = 0x0000000000000002;

    uint64_t last_value = 0xFFFFFFFFFFFFFFFF;

    //
    // On récupère les pointeurs sur nos espaces mémoire
    //

    const uint64_t* in_1 = ifile_1->data.data();
    const uint64_t* in_2 = ifile_2->data.data();
          uint64_t* dest =  o_file->data.data();

    while ( (counterA != nElementsA) && (counterB != nElementsB) )
    {
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
    // Il faut absolument faire un check pour verifier qu'il n'y a pas de redondance dans le flux
    // qui n'est pas encore vide sinon on aura un doublon lors du flush !
    //
    if (counterA == nElementsA) {
        const uint64_t v2 = in_2[counterB];
        if (v2 == last_value) {
            dest[ndst - 1] |= colorDocB;
            counterB += 1;
        }

        for(int i = counterB; i < nElementsB; i += 1){
            dest[ndst++] = in_2[i];
            dest[ndst++] = colorDocB;
        }


    }else if (counterB == nElementsB) {
        const uint64_t v1 = in_1[counterA];
        if (v1 == last_value){
            dest[ndst-1] |= colorDocA;
            counterA  += 1;
        }

        for(int i = counterA; i < nElementsA; i += 1){
            dest[ndst++] = in_1[i];
            dest[ndst++] = colorDocA;
        }
    }

    //
    // On redimentionne le CFileBuffer à la bonne capacité
    //
    o_file->resize_ram_size( ndst );
}
