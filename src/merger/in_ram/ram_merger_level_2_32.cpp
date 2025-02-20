#include "ram_merger_level_1.hpp"

void ram_merge_level_2_32(
        CFileBuffer* ifile_1,
        CFileBuffer* ifile_2,
        CFileBuffer* o_file,
        const int level)
{
    //
    // On calcule la taille maximum que peux avoir le fichier en
    // en sortie de l'étape de fusion des données sachant que les
    // données d'entrée ont des couleurs comprises dans 1 uint64_t
    //

    const int64_t n_uint64_t_A  = ifile_1->uint64_in_ram();
    const int64_t n_uint64_t_B  = ifile_2->uint64_in_ram();
    const int64_t nElementsA    = n_uint64_t_A / 2;
    const int64_t nElementsB    = n_uint64_t_B / 2;
    const int64_t nElements_max = (n_uint64_t_A + n_uint64_t_B);

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

    uint64_t last_value = 0xFFFFFFFFFFFFFFFF;

    //
    // On récupère les pointeurs sur nos espaces mémoire
    //

    const uint64_t* in_1 = ifile_1->data.data();
    const uint64_t* in_2 = ifile_2->data.data();
          uint64_t* dest =  o_file->data.data();

    while ((counterA != nElementsA) && (counterB != nElementsB) )
    {
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
            if (v2 != last_value){
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
    // Il faut absolument faire un check pour verifier qu'il n'y a pas de redondance dans le flux
    // qui n'est pas encore vide sinon on aura un doublon lors du flush !
    //
    if (counterA == nElementsA) {
        const uint64_t v2 = in_2[counterB];
        const uint64_t colorDocB = in_2[counterB + 1] << level; // On decale de 2 bits au level 2, 4 au level 4, etc.
        if (v2 == last_value) {
            dest[ndst - 1] |= colorDocB;
            counterB += 2;
        }

        for(int i = counterB + 1; i < nElementsB; i+= 2)
        {
            dest[ndst++] |= in_2[counterB    ]; // la valeur
            dest[ndst++] |= in_2[counterB + 1]; // sa couleur
        }

    }else if (counterB == nElementsB) {
        const uint64_t v1        = in_1[counterA    ];
        const uint64_t colorDocA = in_1[counterA + 1];
        if (v1 == last_value){
            dest[ndst-1] |= colorDocA;
            counterA  += 2;
        }

        for(int i = counterA + 1; i < nElementsA; i+= 2)
        {
            dest[ndst++] |= in_1[counterA    ]; // la valeur
            dest[ndst++] |= in_1[counterA + 1]; // sa couleur
        }

    }

    //
    // On redimentionne le CFileBuffer à la bonne capacité
    //
    o_file->resize_ram_size( ndst );
}
