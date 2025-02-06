#include "read_fastx_bz2_file.hpp"
#include <bzlib.h>
#include "../../tools/colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
read_fastx_bz2_file::read_fastx_bz2_file(const std::string filen)
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

    int bzerror = 0;
    streaz = BZ2_bzReadOpen( &bzerror, stream, 0, 0, 0, 0 );
    if( bzerror != BZ_OK ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzReadOpen\n");
        reset_section();
        exit(EXIT_FAILURE);
    }

    n_data = BZ2_bzRead ( &bzerror, streaz, buffer, buff_size * sizeof(char) );
    if( bzerror == BZ_STREAM_END ) {
        // cela signifie juste que l'on a atteint la fin du fichier !
        // exit(EXIT_FAILURE);
    }else if( bzerror == BZ_UNEXPECTED_EOF ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) BZ_UNEXPECTED_EOF\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_CONFIG_ERROR ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) BZ_CONFIG_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_SEQUENCE_ERROR ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) BZ_SEQUENCE_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_PARAM_ERROR ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) BZ_PARAM_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_MEM_ERROR ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) BZ_MEM_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_DATA_ERROR_MAGIC ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) BZ_DATA_ERROR_MAGIC\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_DATA_ERROR ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) BZ_DATA_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_IO_ERROR ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) BZ_IO_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_UNEXPECTED_EOF ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) BZ_UNEXPECTED_EOF\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_OUTBUFF_FULL ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) BZ_OUTBUFF_FULL\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror != BZ_OK ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(EE) buff_size = %d\n", buff_size);
        printf("(EE) n_data    = %d\n", n_data);
        reset_section();
        exit(EXIT_FAILURE);
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
read_fastx_bz2_file::~read_fastx_bz2_file()
{
    delete[] buffer;
    BZ2_bzclose(streaz);
    fclose( stream );
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool read_fastx_bz2_file::next_sequence(char* n_kmer)
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
std::tuple<int, bool> read_fastx_bz2_file::next_sequence(char* n_kmer, int buffer_size, const bool _internal_)
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
bool read_fastx_bz2_file::reload()
{
    int reste = n_data - c_ptr;
    for(int i = c_ptr; i < n_data; i += 1)
    {
        buffer[i - c_ptr] = buffer[i];
    }

    int bzerror = 0;
    int n_len = (buff_size - reste);
    int nread = BZ2_bzRead ( &bzerror, streaz, buffer + reste, n_len * sizeof(char) );
    if( bzerror == BZ_STREAM_END ) {
        no_more_load = true;
    }else if( bzerror == BZ_UNEXPECTED_EOF ) {
        error_section();
        printf("(DD) BZ_UNEXPECTED_EOF\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_CONFIG_ERROR ) {
        error_section();
        printf("(DD) BZ_CONFIG_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_SEQUENCE_ERROR ) {
        error_section();
        printf("(DD) BZ_SEQUENCE_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_PARAM_ERROR ) {
        error_section();
        printf("(DD) BZ_PARAM_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_MEM_ERROR ) {
        error_section();
        printf("(DD) BZ_MEM_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_DATA_ERROR_MAGIC ) {
        error_section();
        printf("(DD) BZ_DATA_ERROR_MAGIC\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_DATA_ERROR ) {
        error_section();
        printf("(DD) BZ_DATA_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_IO_ERROR ) {
        error_section();
        printf("(DD) BZ_IO_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_UNEXPECTED_EOF ) {
        error_section();
        printf("(DD) BZ_UNEXPECTED_EOF\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror == BZ_OUTBUFF_FULL ) {
        error_section();
        printf("(DD) BZ_OUTBUFF_FULL\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( bzerror != BZ_OK ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzRead\n");
        printf("(EE) buff_size - reste = %d\n", buff_size - reste);
        printf("(EE) nread             = %d\n", nread);
        reset_section();
        exit(EXIT_FAILURE);
    }
    no_more_load |= ( n_len != nread ); // a t'on atteint la fin du fichier ?
    c_ptr        = 0;                   // on remet a zero le pointeur de lecture
    n_data       = nread + reste;       // on met a jour le nombre de données dans le buffer
    return true;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool read_fastx_bz2_file::is_eof()
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
