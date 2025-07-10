#include "merger_n_files_ge64.hpp"
#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"

void merge_n_files_greater_than_64_colors(
        const std::vector<std::string>& file_list,
        const int64_t n_in_colors,
        const std::string& o_file)
{
    if( n_in_colors < 64 )
    {
        printf("(EE) The number of colors of input files is not in the accepted range (< 64)\n");
        printf("(EE) The current value is : %ld\n", n_in_colors);
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    const int64_t iSize = (n_in_colors + 63) / 64;  // nombre de uint64_t pour coder les couleurs (input)
    const int64_t oSize = iSize * file_list.size(); // nombre de uint64_t pour coder les couleurs (output)

    const int64_t _iBuff_ = (1 + iSize) * 1024; // on a un buffer de 1024 elements (minimizer + couleurs)
    const int64_t _oBuff_ = (1 + oSize) * 1024; // on a un buffer de 1024 elements (minimizer + couleurs)

    //
    // On ouvre tous les fichiers que l'on doit fusionner
    //
    std::vector<stream_reader*> i_files (file_list.size());
    for(size_t i = 0; i < file_list.size(); i += 1)
    {
        stream_reader* f = stream_reader_library::allocate( file_list[i] );
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

    //
    // On cree un vecteur qui va memoriser le nombre de données
    // disponible dans chacun des flux
    //
    std::vector<int64_t> nElements(i_files.size());
    for(size_t i = 0; i < i_files.size(); i += 1)
        nElements[i] = 0;

    //
    // On cree un vecteur qui va memoriser la position courante
    // dans chacun des flux
    //
    std::vector<int64_t> counter(i_files.size());
    for(size_t i = 0; i < i_files.size(); i += 1)
        counter[i] = 0;

    //
    // On cree un vecteur qui va memoriser la localisation des couleurs
    // associée à chacun des flux
    //
    std::vector<int64_t> color_pos(i_files.size());
    for(size_t i = 0; i < i_files.size(); i += 1)
        color_pos[i] = i * iSize;

    //
    // On ouvre le fichier de destination
    //
    stream_writer* fdst = stream_writer_library::allocate( o_file );
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
              //nElements[i] = fread(i_buffer[i], sizeof(uint64_t), _iBuff_, i_files[i]);
                nElements[i] = i_files[i]->read(i_buffer[i], sizeof(uint64_t), _iBuff_);
                counter  [i] = 0;
                if( nElements[i] == 0 )
                {
                    // On est arrivé à la fin du fichier, donc on suppirme le flux
                    // du processus de fusion...
                  //fclose         ( i_files[i]);
                    delete i_files[i];
                    i_files.erase  ( i_files.begin()   + i );
                    i_buffer.erase ( i_buffer.begin()  + i );
                    nElements.erase( nElements.begin() + i );
                    color_pos.erase( color_pos.begin() + i );
                    counter.erase  ( counter.begin()   + i );
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
            }

            //
            //
            //
            const uint64_t* stream = i_buffer[curr_index] + counter[curr_index]; // ptr sur le flux "gagnant", a la position du minimizer
            if (curr_value != last_value){
                dest[ndst++]         = curr_value;  // on memorise la valeur
                for(int y = 0; y < oSize; y += 1)   // on ajoute toutes les couleurs
                    dest[ndst++] = 0;
                for(int y = 0; y < iSize; y += 1)   // on recopie les couleurs provenant du flux d'entrée
                    dest[ndst - oSize + color_pos[curr_index] + y] = stream[1 + y]; // on saute la valeur du minimizer
                last_value           = curr_value;
                counter[curr_index] += 1 + iSize;  // on a lu le minimizer + ses couleurs dans le flux
            }else{
                for(int y = 0; y < iSize; y += 1)   // on recopie les couleurs provenant du flux d'entrée
                    dest[ndst - oSize + color_pos[curr_index] + y] = stream[1 + y]; // on saute la valeur du minimizer
                last_value           = curr_value;
                counter[curr_index] += 1 + iSize;  // on a lu le minimizer + ses couleurs dans le flux
            }

            //
            // Si le buffer de sortie est full alors on le flush ! Mais on ne doit pas supprimer le dernier
            // element car on peut avoir besoin de mettre a jour ses couleurs à l'itération suivante
            //
            if (ndst == _oBuff_) {
                //fwrite(dest, sizeof(uint64_t), ndst - 1 - oSize, fdst);
                fdst->write(dest, sizeof(uint64_t), ndst - 1 - oSize);
                for(int y = 0; y < 1 + oSize; y += 1)
                    dest[y] = dest[ndst - 1 - oSize + y];
                ndst = 1 + oSize;
            }

        }while( counter[curr_index] != nElements[curr_index] );

    }

    //
    // On flush les données restantes avant de quitter
    //
    if (ndst != 0) {
        fdst->write(dest, sizeof(uint64_t), ndst);
        //fwrite(dest, sizeof(uint64_t), ndst, fdst);
        ndst = 0;
    }

    //
    // On ferme le fichier de sortie et on détruit le buffer
    //
    //fclose( fdst  );
    delete [] dest;
    delete fdst;    // on détruit le fichier de sortie (fclose)
}
