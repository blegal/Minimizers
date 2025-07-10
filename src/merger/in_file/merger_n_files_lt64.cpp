#include "merger_n_files_lt64.hpp"
#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"

void merge_n_files_less_than_64_colors(
        const std::vector<std::string>& file_list,
        const std::string& o_file)
{
    if( (file_list.size() < 1) || (file_list.size() > 64) )
    {
        printf("(EE) The number of files to merge is not in the accepted range (1 <= x <= 64)\n");
        printf("(EE) The current value is : %ld\n", file_list.size());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    const int64_t _iBuff_ = 64 * 1024;
    const int64_t _oBuff_ = _iBuff_;

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
    // On cree un vecteur qui va memoriser la couleur associée
    // a chacun des flux
    //
    const uint64_t one_bit = 1;
    std::vector<int64_t> color(i_files.size());
    for(size_t i = 0; i < i_files.size(); i += 1)
        color[i] = one_bit << i;

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
                nElements[i] = i_files[i]->read(i_buffer[i], sizeof(uint64_t), _iBuff_);
                //nElements[i] = fread(i_buffer[i], sizeof(uint64_t), _iBuff_, i_files[i]);
                counter  [i] = 0;
                if( nElements[i] == 0 )
                {
                    // On est arrivé à la fin du fichier, donc on suppirme le flux
                    // du processus de fusion...
                    delete i_files[i];
                  //fclose         ( i_files[i]             );
                    i_files.erase  ( i_files.begin()   + i );
                    i_buffer.erase ( i_buffer.begin()  + i );
                    nElements.erase( nElements.begin() + i );
                    counter.erase  ( counter.begin()   + i );
                    color.erase    ( color.begin()     + i );
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
            curr_value = 0xFFFFFFFFFFFFFFFF; // la première du premier flux
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
                dest[ndst++]         = curr_value;        // on memorise la valeur
                dest[ndst++]         = color[curr_index]; // on memorise la couleur
                last_value           = curr_value;        // on retient la nouvelle valeur
                counter[curr_index] += 1;                 // on n'a lu que le minimizer dans le flux
            }else{
//              printf("(1) curr_value = %16.16X | ndst = %4d | curr_index = %d |\n", curr_value, ndst, curr_index);
                last_value           = curr_value;
                dest[ndst-1]        |= color[curr_index]; // on ajoute la couleur a la donnée précédement stockée
                counter[curr_index] += 1;                 // on n'a lu que le minimizer dans le flux
            }

            //
            // Si le buffer de sortie est full alors on le flush ! Mais on ne doit pas supprimer les 2 dernieres
            // valeurs car on peut avoir besoin de mettre a jour les couleurs à l'itération suivante
            //
            if (ndst == _oBuff_) {
              //fwrite(dest, sizeof(uint64_t), ndst - 2, fdst);
                fdst->write(dest, sizeof(uint64_t), ndst - 2);
                dest[0] = dest[ndst-2]; // on recupere le dernier minimizer inséré
                dest[1] = dest[ndst-1]; // on recupere la derniere couleur insérée
                ndst = 2;
            }

        }while( counter[curr_index] != nElements[curr_index] );
    }

    //
    // On flush les données restantes avant de quitter
    //
    if (ndst != 0) {
      //fwrite(dest, sizeof(uint64_t), ndst, fdst);
        fdst->write(dest, sizeof(uint64_t), ndst);
        ndst = 0;
    }

    //
    // On ferme le fichier de sortie et on détruit le buffer
    //
  //fclose( fdst  );
    delete [] dest; // on detruit le buffer interne
    delete fdst;    // on détruit le fichier de sortie (fclose)
}
