#include "read_fastx_lz4_file.hpp"
#include "../../tools/colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
read_fastx_lz4_file::read_fastx_lz4_file(const std::string filen)
{
    buffer = new char[buff_size];

    stream = fopen( filen.c_str(), "r" );
    if( stream == NULL )
    {
        error_section();
        printf("(EE) File does not exist (%s))\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    LZ4F_errorCode_t ret = LZ4F_readOpen(&lz4fRead, stream);
    if (LZ4F_isError(ret)) {
        error_section();
        printf("(EE) LZ4F_readOpen error: %s\n", LZ4F_getErrorName(ret));
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    n_data = LZ4F_read (lz4fRead, buffer, buff_size * sizeof(char) );
    if (LZ4F_isError(ret)) {
        error_section();
        printf("(EE) LZ4F_read error: %s\n", LZ4F_getErrorName(ret));
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

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
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
read_fastx_lz4_file::~read_fastx_lz4_file()
{
    delete[] buffer;
    LZ4F_readClose(lz4fRead);
    fclose( stream );
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool read_fastx_lz4_file::next_sequence(char* n_kmer)
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
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
std::tuple<int, bool> read_fastx_lz4_file::next_sequence(char* n_kmer, int buffer_size, const bool _internal_)
{
    //
    // Le fichier est fini, donc rien à faire
    //
    if( file_ended == true )
    {
        return {0, true}; // aucun octet n'est disponible
    }

    //
    // On cherche le caractere de fin de ligne
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

    //
    // Le buffer se termine t'il avant que l'on ai recontré un char fin de ligne
    //
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

    //
    // On se trouve en début d'une ligne du fichier on doit donc
    // regarder si cette ligne c'est un commentaire ou une sequence de nucléotides.
    // Si c'est un commentaire alors on passe a la ligne suivante (appel recursif)
    //
    bool new_seq = (buffer[c_ptr] == '>') || (buffer[c_ptr] == '+') || (buffer[c_ptr] == '@') || (buffer[c_ptr] == '=');
    if( new_seq == true )
    {
        c_ptr      = pos_nline + 1;
        file_ended = (c_ptr >= n_data) && (n_data != buff_size);
        return next_sequence(n_kmer, buffer_size, true);
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
    // On indique que la ligne actuelle est la continuité de la séquence précédente
    //
    return {cnt, _internal_};
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool read_fastx_lz4_file::reload()
{
    int reste = n_data - c_ptr;
    for(int i = c_ptr; i < n_data; i += 1)
    {
        buffer[i - c_ptr] = buffer[i];
    }

    const int n_len = (buff_size - reste);
    const LZ4F_errorCode_t nread = LZ4F_read (lz4fRead, buffer + reste, n_len * sizeof(char) );
    if (LZ4F_isError(nread)) {
        error_section();
        printf("(EE) LZ4F_read error: %s\n", LZ4F_getErrorName(nread));
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    no_more_load |= ( n_len != static_cast<int>(nread) ); // a t'on atteint la fin du fichier ?
    c_ptr        = 0;                       // on remet a zero le pointeur de lecture
    n_data       = nread + reste;           // on met a jour le nombre de données dans le buffer
    return true;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool read_fastx_lz4_file::is_eof()
{
    if( (no_more_load == true) && (c_ptr == n_data) )
        return true;
    return false;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
