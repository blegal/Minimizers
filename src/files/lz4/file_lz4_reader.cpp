#include "file_lz4_reader.hpp"
#include "../../tools/colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
file_lz4_reader::file_lz4_reader(const std::string& filen)
{
    //
    // Ouverture du fichier en mode lecture !
    //
    stream = fopen( filen.c_str(), "r" );
    if( stream == NULL )
    {
        printf("(EE) File does not exist (%s))\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }
    is_open     = true; // file

    //
    // Ouverture du stream LZ4 en mode lecture !
    //
    LZ4F_errorCode_t ret = LZ4F_readOpen(&lz4fRead, stream);
    if (LZ4F_isError(ret)) {
        printf("(EE) LZ4F_readOpen error: %s\n", LZ4F_getErrorName(ret));
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
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
file_lz4_reader::~file_lz4_reader()
{
    if( stream_open )
        LZ4F_readClose(lz4fRead);
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
bool file_lz4_reader::isOpen ()
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
bool file_lz4_reader::isClose()
{
    return (!is_open) || (!stream_open);
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool file_lz4_reader::is_eof()
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
int  file_lz4_reader::read(char* buffer, int eSize, int eCount)
{
    const LZ4F_errorCode_t nread = LZ4F_read (lz4fRead, buffer, eCount * eSize );
    if (LZ4F_isError(nread)) {
        error_section();
        printf("(EE) LZ4F_read error: %s\n", LZ4F_getErrorName(nread));
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }
    is_oef |= ( (eCount * eSize) != nread ); // a t'on atteint la fin du fichier ?
    return nread;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//