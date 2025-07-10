#include "ram_merger_n_files.hpp"

void ram_merge_n_files(
        const std::vector<std::string>& file_list,
        const std::string& o_file)
{
//    std::cout << "merging(" << ifile_1 << ", " << ifile_2 << ") => " << o_file << std::endl;

    const int64_t _iBuff_ = 64 * 1024;
    const int64_t _oBuff_ = _iBuff_;

    //
    // On ouvre tous les fichiers que l'on doit fusionner
    //
    std::vector<FILE*> i_files (file_list.size());
    for(size_t i = 0; i < file_list.size(); i += 1)
    {
        FILE *f = fopen(file_list[i].c_str(), "r");
        if( f == NULL )
        {
            printf("(EE) File does not exist (%s))\n", file_list[i].c_str());
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit( EXIT_FAILURE );
        }
        i_files[i] = f;
    }

    //
    // On cree les buffers pour tamponner les lectures
    //
    std::vector<uint64_t*> i_buffer(i_files.size());
    for(size_t i = 0; i < i_files.size(); i += 1)
        i_buffer[i] = new uint64_t[_iBuff_];


    //
    // On cree le buffer pour les données de sortie
    //
    uint64_t *dest = new uint64_t[_oBuff_];


    std::vector<int64_t> nElements(i_files.size());
    for(size_t i = 0; i < i_files.size(); i += 1)
        nElements[i] = 0;


    std::vector<int64_t> counter(i_files.size());
    for(size_t i = 0; i < i_files.size(); i += 1)
        counter[i] = 0;

    //
    // On ouvre le fichier de destination
    //
    FILE *fdst  = fopen(o_file.c_str(),   "w");
    if( fdst == NULL )
    {
        printf("(EE) File does not exist (%s))\n", o_file.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    //
    // On cree le compteur pour le buffer de sortie
    //
    int64_t ndst        = 0; // nombre de données écrites dans le flux
    uint64_t last_value = 0xFFFFFFFFFFFFFFFF;
    while (true)
    {

        //
        // On verifie sur tous les fichiers ouverts que l'on a des données
        //
        for(size_t i = 0; i < i_files.size(); i += 1)
        {
            //
            // Doit'on recharger des données dans le flux ?
            //
            if (counter[i] == nElements[i])
            {
                nElements[i] = fread(i_buffer[i], sizeof(uint64_t), _iBuff_, i_files[i]);
                counter  [i] = 0;
                if( nElements[i] == 0 )
                {
                    // On est arrivé à la fin du fichier, donc on suppirme le flux
                    // du processus de fusion...
                    i_files.erase  ( i_files.begin()   + i );
                    i_buffer.erase ( i_buffer.begin()  + i );
                    nElements.erase( nElements.begin() + i );
                    i -= 1;
                }
            }
        }

        //
        // Si il n'y a plus de flux ouvert, cela signifie que le processus de fusion est terminé
        //
        if(i_files.size() == 0)
            break;

        uint64_t curr_index;
        uint64_t curr_value;
        do{
            //
            // On recherche dans tous les flux ouverts la valeur minimum
            //
            curr_index = -1;
            curr_value = 0xFFFFFFFFFFFFFFFF; // la premmière du premier flux
            for(size_t i = 0; i < i_files.size(); i += 1)
            {
                const int       pos = counter [i];
                const uint64_t* buf = i_buffer[i];
                const uint64_t  val = buf[ pos ];
                if( val < curr_value )
                {
                    curr_value = val;
                    curr_index = i;
                }
//              printf("(+) val = %16.16X | curr_value = %16.16X | curr_index = %2d |\n", val, curr_value, curr_index);
            }

            //
            //
            //
            if (curr_value != last_value){
//              printf("(1) curr_value = %16.16X | ndst = %4d | curr_index = %d |\n", curr_value, ndst, curr_index);
                dest[ndst++]         = curr_value;
                last_value           = curr_value;
                counter[curr_index] += 1;
            }else{
//              printf("(1) curr_value = %16.16X | ndst = %4d | curr_index = %d |\n", curr_value, ndst, curr_index);
                last_value           = curr_value;
                counter[curr_index] += 1;
            }

//          if( counter[curr_index] == 66536 )
//              printf("oups !\n");


            //
            // Si le buffer de sortie est full alors on le flush !
            //
            if (ndst == _oBuff_) {
                fwrite(dest, sizeof(uint64_t), ndst, fdst);
                ndst = 0;
            }

        }while( counter[curr_index] != nElements[curr_index] );

    }

    //
    // On flush les données restantes avant de quitter
    //
    if (ndst != 0) {
        fwrite(dest, sizeof(uint64_t), ndst, fdst);
        ndst = 0;
    }

    //
    // On ferme le fichier de sortie et on détruit le buffer
    //
    fclose( fdst  );
    delete [] dest;
}
