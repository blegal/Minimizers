#include "stream_raw_writer.hpp"
#include "../../../tools/colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
stream_raw_writer::stream_raw_writer(const std::string& filen)
{
    //
    // Ouverture du fichier en mode lecture !
    //
    stream = fopen( filen.c_str(), "w" );
    if( stream == NULL )
    {
        error_section();
        printf("(EE) It is impossible to create the file (%s))\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }
    is_fopen = true; // file
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
stream_raw_writer::~stream_raw_writer()
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
bool stream_raw_writer::is_open ()
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
int stream_raw_writer::write(void* buffer, int eSize, int eCount)
{
    const int nwrite = fwrite(buffer, eSize, eCount, stream);
    if( nwrite != eCount ) // fwrite retourne le nombre d'éléments copiés et non le nombre de bytes !
    {
        error_section();
        printf("(EE) An error occured during the data write task:\n");
        printf("(EE) - Amount of data to write (eSize)  : %d\n", eSize);
        printf("(EE) - Amount of data to write (eCount) : %d\n", eCount);
        printf("(EE) - Amount of written data  (nwrite) : %d\n", nwrite);
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }
    return nwrite; // nombre d'éléments de taille eSize
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void stream_raw_writer::close()
{
    fflush( stream );
    fclose( stream );
    is_fopen = false;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//