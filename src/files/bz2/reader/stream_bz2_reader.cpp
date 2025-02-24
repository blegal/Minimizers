#include "stream_bz2_reader.hpp"
#include "../../../tools/colors.hpp"
#include <bzlib.h>
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
stream_bz2_reader::stream_bz2_reader(const std::string& filen)
{
    //
    // Ouverture du stream LZ4 en mode lecture !
    //
    bzf = BZ2_bzopen  ( filen.c_str(), "rb" );
    if( bzf == NULL ) {
        error_section();
        printf("(EE) An error happens during BZ2_bzopen\n");
        reset_section();
        exit(EXIT_FAILURE);
    }

    is_fopen = true;
    is_foef  = false;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
stream_bz2_reader::~stream_bz2_reader()
{
    if( is_open() == true )
        close();
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool stream_bz2_reader::is_open ()
{
    return is_fopen;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool stream_bz2_reader::is_eof()
{
    return is_foef;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int stream_bz2_reader::read(void* buffer, int eSize, int eCount)
{
    const int nread = BZ2_bzread ( bzf, buffer, eSize * eCount);
    if( nread < 0 )
    {
        error_section();
        printf("(EE) Something strange happened (%d))\n", nread);
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    // a t'on atteint la fin du fichier ?
    is_foef |= ( (eCount * eSize) != nread );
    return (nread / eSize); // nombre d'éléments lu et NON PAS le nombre de bytes !
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void stream_bz2_reader::close()
{
    BZ2_bzflush( bzf );
    BZ2_bzclose( bzf );
    is_fopen = false;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
