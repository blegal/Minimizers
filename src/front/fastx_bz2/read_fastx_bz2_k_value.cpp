#include "read_fastx_bz2_k_value.hpp"
#include <bzlib.h>
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
#define buffer_size (1024*1024)
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int read_fastx_bz2_k_value(std::string filename)
{
    FILE* stream = fopen( filename.c_str(), "r" );
    if( stream == NULL )
    {
        printf("(EE) File does not exist (%s))\n", filename.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    BZFILE* streaz;
    int bzerror = 0;
    streaz = BZ2_bzReadOpen( &bzerror, stream, 0, 0, 0, 0 );
    if( bzerror != BZ_OK ) {
        printf("(EE) An error happens during BZ2_bzReadOpen\n");
        exit(EXIT_FAILURE);
    }

    uint64_t end_of_line = 0;
    char buffer[buffer_size];

    int nread = buffer_size;
    while ( nread == buffer_size )
    {
//      nread = fread(buffer, sizeof(char), buffer_size, f);
        nread = BZ2_bzRead ( &bzerror, streaz, buffer, buffer_size * sizeof(char) );
        if( bzerror == BZ_STREAM_END ) {
            exit(EXIT_FAILURE);
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
            printf("(EE) An error happens during BZ2_bzRead\n");
            printf("(EE) BZ_DATA_ERROR_MAGIC\n");
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
            printf("(EE) buff_size = %d\n", buffer_size);
            printf("(EE) n_data    = %d\n", nread);
            exit(EXIT_FAILURE);
        }

        for(int x = 0; x < nread; x += 1)
        {
            if( buffer[x] == '\n' )
            {
                nread = 0;  // on arret de cherche la fin de la premiere sequence
                break;      // car on est arrivé sur une fin de ligne
            }
            end_of_line += 1;
        }
    }

    BZ2_bzclose(streaz);
    fclose( stream );

    return end_of_line;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
