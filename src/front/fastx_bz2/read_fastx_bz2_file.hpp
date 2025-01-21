#pragma once
#include "../file_reader.hpp"
#include <bzlib.h>

class read_fastx_bz2_file : public file_reader
{
private:
    const int buff_size = 64 * 1024 * 1024;
    char* buffer;
    int n_data   = 0;
    int c_ptr    = 0;
    int n_lines        = 0;
    bool no_more_load  = false;
    bool file_ended    = false;
    FILE*   stream;
    BZFILE* streaz;

public:
    read_fastx_bz2_file(const std::string filen)
    {
        buffer = new char[buff_size];

        stream = fopen( filen.c_str(), "r" );
        if( stream == NULL )
        {
            printf("(EE) File does not exist (%s))\n", filen.c_str());
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit( EXIT_FAILURE );
        }

        int bzerror = 0;
        streaz = BZ2_bzReadOpen( &bzerror, stream, 0, 0, 0, 0 );
        if( bzerror != BZ_OK ) {
            printf("(EE) An error happens during BZ2_bzReadOpen\n");
            exit(EXIT_FAILURE);
        }

        n_data = BZ2_bzRead ( &bzerror, streaz, buffer, buff_size * sizeof(char) );

        if( bzerror == BZ_STREAM_END ) {
            // cela signifie juste que l'on a atteint la fin du fichier !
            // exit(EXIT_FAILURE);
        }else if( bzerror == BZ_UNEXPECTED_EOF ) {
            printf("(DD) BZ_UNEXPECTED_EOF\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_CONFIG_ERROR ) {
            printf("(DD) BZ_CONFIG_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_SEQUENCE_ERROR ) {
            printf("(DD) BZ_SEQUENCE_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_PARAM_ERROR ) {
            printf("(DD) BZ_PARAM_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_MEM_ERROR ) {
            printf("(DD) BZ_MEM_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_DATA_ERROR_MAGIC ) {
            printf("(DD) BZ_DATA_ERROR_MAGIC\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_DATA_ERROR ) {
            printf("(DD) BZ_DATA_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_IO_ERROR ) {
            printf("(DD) BZ_IO_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_UNEXPECTED_EOF ) {
            printf("(DD) BZ_UNEXPECTED_EOF\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_OUTBUFF_FULL ) {
            printf("(DD) BZ_OUTBUFF_FULL\n");
            exit(EXIT_FAILURE);
        }else if( bzerror != BZ_OK ) {
            printf("(EE) An error happens during BZ2_bzRead\n");
            printf("(EE) buff_size = %d\n", buff_size);
            printf("(EE) n_data    = %d\n", n_data);
            exit(EXIT_FAILURE);
        }

    }

    //////////////////////////////////////////////////////////////////////////////////////////

    ~read_fastx_bz2_file()
    {
        delete[] buffer;
        BZ2_bzclose(streaz);
        fclose( stream );
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

    //////////////////////////////////////////////////////////////////////////////////////////

    bool reload()
    {
        int reste = n_data - c_ptr;
        for(int i = c_ptr; i < n_data; i += 1)
        {
            buffer[i - c_ptr] = buffer[i];
        }
//      int nread = fread(buffer + reste, sizeof(char), buff_size - reste, f);
        int bzerror = 0;
        int nread = BZ2_bzRead ( &bzerror, streaz, buffer + reste, (buff_size - reste) * sizeof(char) );
        if( bzerror == BZ_STREAM_END ) {
            no_more_load = true;
        }else if( bzerror == BZ_UNEXPECTED_EOF ) {
            printf("(DD) BZ_UNEXPECTED_EOF\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_CONFIG_ERROR ) {
            printf("(DD) BZ_CONFIG_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_SEQUENCE_ERROR ) {
            printf("(DD) BZ_SEQUENCE_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_PARAM_ERROR ) {
            printf("(DD) BZ_PARAM_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_MEM_ERROR ) {
            printf("(DD) BZ_MEM_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_DATA_ERROR_MAGIC ) {
            printf("(DD) BZ_DATA_ERROR_MAGIC\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_DATA_ERROR ) {
            printf("(DD) BZ_DATA_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_IO_ERROR ) {
            printf("(DD) BZ_IO_ERROR\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_UNEXPECTED_EOF ) {
            printf("(DD) BZ_UNEXPECTED_EOF\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_OUTBUFF_FULL ) {
            printf("(DD) BZ_OUTBUFF_FULL\n");
            exit(EXIT_FAILURE);
        }else if( bzerror != BZ_OK ) {
            printf("(EE) An error happens during BZ2_bzRead\n");
            printf("(EE) buff_size - reste = %d\n", buff_size - reste);
            printf("(EE) nread             = %d\n", nread);
            exit(EXIT_FAILURE);
        }
        no_more_load |= ( n_data != buff_size ); // a t'on atteint la fin du fichier ?
        c_ptr        = 0;                       // on remet a zero le pointeur de lecture
        n_data       = nread + reste;           // on met a jour le nombre de données dans le buffer
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
