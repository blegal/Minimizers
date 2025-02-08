#include "file_bz2_reader.hpp"
#include "../../tools/colors.hpp"
#include <bzlib.h>
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
file_bz2_reader::file_bz2_reader(const std::string& filen)
{
    //
    // Ouverture du fichier en mode lecture !
    //
    stream = fopen( filen.c_str(), "r" );
    if( stream == NULL )
    {
        error_section();
        printf("(EE) File does not exist (%s))\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }
    is_open     = true; // file

    //
    // Ouverture du stream LZ4 en mode lecture !
    //
    int bzerror = 0;
    streaz = BZ2_bzReadOpen( &bzerror, stream, 0, 0, 0, 0 );
    if( bzerror != BZ_OK ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzReadOpen\n");
        reset_section();
        exit(EXIT_FAILURE);
    }

    stream_open = true;
    is_oef      = false;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
file_bz2_reader::~file_bz2_reader()
{
    if( stream_open )
        BZ2_bzclose(streaz);
    if( is_open )
        fclose( stream );
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool file_bz2_reader::isOpen ()
{
    return is_open && stream_open;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool file_bz2_reader::isClose()
{
    return !isOpen();
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool file_bz2_reader::is_eof()
{
    return is_oef;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int file_bz2_reader::read(char* buffer, int eSize, int eCount)
{
    int bzerror = 0;
    const int nread = BZ2_bzRead ( &bzerror, streaz, buffer, eSize * eCount);
    if( bzerror == BZ_STREAM_END ) {
        is_oef = true;
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
        printf("(EE) nread             = %d\n", nread);
        reset_section();
        exit(EXIT_FAILURE);
    }
    // a t'on atteint la fin du fichier ?
    is_oef |= ( (eCount * eSize) != nread );
    return nread;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//