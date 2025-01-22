#pragma once
#include "../file_reader.hpp"

class read_fastx_file : public file_reader
{
private:
    const int buff_size = 64 * 1024 * 1024; // taille du tampon entre le fichier et l'application
    char* buffer;
    int n_data   = 0;
    int c_ptr    = 0;
    int n_lines        = 0;
    bool no_more_load  = false;
    bool file_ended    = false;
    FILE* f;

public:
    read_fastx_file(const std::string filen)
    {
        buffer = new char[buff_size];

        f = fopen( filen.c_str(), "r" );
        if( f == NULL )
        {
            printf("(EE) File does not exist (%s))\n", filen.c_str());
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit( EXIT_FAILURE );
        }
        n_data = fread(buffer, sizeof(char), buff_size, f);

        //
        // Les fichiers fasta sont normalement équipé d'une en-tete que l'on peut directement
        // skipper...
        //
        if( (buffer[c_ptr] == '>') || (buffer[c_ptr] == '+') || (buffer[c_ptr] == '@') || (buffer[c_ptr] == '>') )
        {
            for(int i = c_ptr; i < n_data; i += 1) {
                if (buffer[i] == '\n')
                {
                    c_ptr = i + 1; // on se positionne sur le 1er caractere de la prochaine ligne
                    break;
                }
            }
        }
        // Fin de skip du premier commentaire

    }

    //////////////////////////////////////////////////////////////////////////////////////////

    ~read_fastx_file()
    {
        delete[] buffer;
        fclose( f );
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    bool next_sequence(char* n_kmer)
    {
        if( file_ended == true )
        {
            return false;
        }

        //
        // On regarde si l'on a encore une entrée dans le buffer
        //
        int pos_nline = -1;
        for(int i = c_ptr; i < n_data; i += 1)
        {
            if(buffer[i] == '\n')
            {
                pos_nline = i;
                break;
            }
        }

        if( pos_nline == -1 )
        {
            if( is_eof() == true )
                return false;

            if( reload() == true )
            {
                return next_sequence(n_kmer);
            }else{
                return false;
            }
        }


        int cnt = 0;
        while( buffer[c_ptr] != '\n' )
        {
            n_kmer[cnt++] = buffer[c_ptr++];
        }
        n_kmer[cnt] = 0;
        c_ptr += 1; // on passe le retour à la ligne

        n_lines += 1;

        file_ended = (c_ptr == n_data) && (n_data != buff_size);

        return true;
    }

    std::tuple<int, bool> next_sequence(char* n_kmer, int buffer_size)
    {
        //
        // Le fichier est fini, donc rien à faire
        //
        if( file_ended == true )
        {
            return {0, false}; // aucun octet n'est disponible
        }

        //
        // Normalement on se trouve en début d'une ligne du fichier on doit donc regarder si c'est un
        // commentaire ou une sequence de nucléotide
        //
        while(true)
        {
            if( (buffer[c_ptr] == '>') || (buffer[c_ptr] == '+') || (buffer[c_ptr] == '@') || (buffer[c_ptr] == '>') )
            {
                //
                // On est dans un commentaire, on doit sauter la ligne
                //
                int pos_nline = -1;
                for(int i = c_ptr; i < n_data; i += 1) {
                    if (buffer[i] == '\n')
                    {
                        c_ptr = i + 1; // on se positionne sur le 1er caractere de la prochaine ligne
                        break;
                    }
                }
                //
                // on n'a plus assez de data dans le buffer...
                //
                if( pos_nline == -1 )
                {
                    if( is_eof() == true )
                        return {0, false};                          // on est arrivé au bout du fichier
                    if( reload() == true ){
                        return next_sequence(n_kmer, buffer_size);  // apres le rechargement on a des données à lire
                    }else{
                        return {0, false};                          // on est arrivé au bout du fichier
                    }
                }
            }else{
                break; // la prochaine ligne n'est pas un commentaire, on peut travailler normalement
            }
        }

        //
        // On regarde si l'on a encore une entrée complete dans le buffer
        //
        int pos_nline = -1;
        for(int i = c_ptr; i < n_data; i += 1)
        {
            if(buffer[i] == '\n')
            {
                pos_nline = i; // il n'y a rien derniere donc c'est OK
                break;
            }
        }

        if( pos_nline == -1 )                               // On n'a pas trouvé de retour à la ligne
        {                                                   // avec des données dedans
            if( is_eof() == true )
                return {0, false};                          // on est arrivé au bout du fichier
            if( reload() == true ){
                return next_sequence(n_kmer, buffer_size);  // apres le rechargement on a des données à lire
            }else{
                return {0, false};                          // on est arrivé au bout du fichier
            }
        }

        int cnt = 0;                                        // On recoipie les données que l'on souhaite
        while( buffer[c_ptr] != '\n' )                      // transmettre a la fonction appelante
        {
            n_kmer[cnt++] = buffer[c_ptr++];
        }
        n_kmer[cnt] = 0;                                    // On rajoute un caractere fin de string
        c_ptr      += 1;                                    // pour des raison de compatibilité (strlen)

        n_lines    += 1;

        //
        // on detecte le moment ou l'on arrive a la fin du fichier
        //
        file_ended = (c_ptr == n_data) && (n_data != buff_size);

        //
        // On doit preciser si on est arrivé à la fin de la séquence de nucléotide
        // - fin de fichier
        // - la ligne suivante est un commentaire
        //
        if( file_ended == true )
        {
            return {cnt, true};
        }

        //
        // c'est penible, on doit recharger le buffer pour avoir la réponse...
        //
        if( c_ptr == n_data )
        {
            if( reload() == false ) // donc fin de fichier
                return {cnt, true}; // c'est la fin de la sequence de nucleotides
        }

        bool is_comment  = (buffer[c_ptr] == '>');  // sinon on regarde si la prochaine ligne
             is_comment |= (buffer[c_ptr] == '+');  // est un commentaire car cela signifie
             is_comment |= (buffer[c_ptr] == '@');  // aussi que c'est la fin de la
             is_comment |= (buffer[c_ptr] == '>');  // sequence de nucleotides

        return {cnt, is_comment};
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    bool reload()
    {
        int reste = n_data - c_ptr;
        for(int i = c_ptr; i < n_data; i += 1)  // on fait vieillir les données qui n'ont pas encore été consommées
        {
            buffer[i - c_ptr] = buffer[i];
        }
        int nread = fread(buffer + reste, sizeof(char), buff_size - reste, f);
//        printf("[%6d] ASKED        : %d\n", n_lines, buff_size - reste);
//        printf("[%6d] READ         : %d\n", n_lines, nread);
        no_more_load = ( n_data != buff_size ); // a t'on atteint la fin du fichier ?
//        printf("[%6d] no_more_load : %d\n", n_lines, no_more_load);
        c_ptr        = 0;                       // on remet a zero le pointeur de lecture
        n_data       = nread + reste;           // on met a jour le nombre de données dans le buffer
//        printf("[%6d] n_data       : %d\n", n_lines, n_data);
//        printf("[%6d] END OF DATA RELOADING !\n", n_lines);
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    bool is_eof()
    {
        if( (no_more_load == true) && (c_ptr == n_data) )
            return true;
        return false;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
};
