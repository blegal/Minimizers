#include "count_lines_fastx_bz2.hpp"
#include <bzlib.h>
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
#define buffer_size (4096)
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
uint64_t count_lines_fastx_bz2(std::string filename)
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

    uint64_t n_sequences = 0;
    char buffer[buffer_size];

    int nread = buffer_size;
    while ( nread == buffer_size )
    {

//      nread = fread(buffer, sizeof(char), buffer_size, f);
        nread = BZ2_bzRead ( &bzerror, streaz, buffer, buffer_size * sizeof(char) );
        if( bzerror == BZ_STREAM_END ) {
            //printf("(II) BZ_STREAM_END\n");
            // c'est juste normal a la fin de la lecture du fichier
        }else if( bzerror == BZ_UNEXPECTED_EOF ) {
            printf("(EE) BZ_UNEXPECTED_EOF\n");
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_CONFIG_ERROR ) {
            printf("(EE) BZ_CONFIG_ERROR\n");
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_SEQUENCE_ERROR ) {
            printf("(EE) BZ_SEQUENCE_ERROR\n");
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_PARAM_ERROR ) {
            printf("(EE) BZ_PARAM_ERROR\n");
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_MEM_ERROR ) {
            printf("(EE) BZ_MEM_ERROR\n");
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_DATA_ERROR_MAGIC ) {
            printf("(EE) BZ_DATA_ERROR_MAGIC\n");
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_DATA_ERROR ) {
            printf("(EE) BZ_DATA_ERROR\n");
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_IO_ERROR ) {
            printf("(EE) BZ_IO_ERROR\n");
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_UNEXPECTED_EOF ) {
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            printf("(EE) BZ_UNEXPECTED_EOF\n");
            exit(EXIT_FAILURE);
        }else if( bzerror == BZ_OUTBUFF_FULL ) {
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            printf("(EE) BZ_OUTBUFF_FULL\n");
            exit(EXIT_FAILURE);
        }else if( bzerror != BZ_OK ) {
            printf("(EE) An error happens during BZ2_bzRead\n");
            printf("(EE) buff_size = %d\n", buffer_size);
            printf("(EE) n_data    = %d\n", nread);
            exit(EXIT_FAILURE);
        }

        for(int x = 0; x < nread; x += 1)
            n_sequences += (buffer[x] == '\n');
    }

    BZ2_bzclose(streaz);
    fclose( stream );

    return n_sequences;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//