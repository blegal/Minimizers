#include "file_gz_reader.hpp"
#include "../../tools/colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
file_gz_reader::file_gz_reader(const std::string& filen)
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
    streaz = gzdopen(fileno(stream), "r");
    if( streaz == NULL ) {
        error_section();
        printf("(EE) An error happens during gzdopen\n");
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
file_gz_reader::~file_gz_reader()
{
    if( stream_open )
        gzclose(streaz);
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
bool file_gz_reader::isOpen ()
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
bool file_gz_reader::isClose()
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
bool file_gz_reader::is_eof()
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
int file_gz_reader::read(char* buffer, int eSize, int eCount)
{
    const int n_data = gzread(streaz, buffer,  eSize * eCount);
    if( n_data == Z_STREAM_END ) {
        is_oef = true;
    }else if( n_data == Z_STREAM_ERROR ) {
        error_section();
        printf("(EE) Z_STREAM_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( n_data == Z_DATA_ERROR ) {
        error_section();
        printf("(EE) Z_DATA_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( n_data == Z_NEED_DICT ) {
        error_section();
        printf("(EE) Z_NEED_DICT\n");
        reset_section();
        exit(EXIT_FAILURE);
    }else if( n_data == Z_MEM_ERROR ) {
        error_section();
        printf("(EE) Z_MEM_ERROR\n");
        reset_section();
        exit(EXIT_FAILURE);
    }

    is_oef |= ( (eCount * eSize) != n_data ); // a t'on atteint la fin du fichier ?
    return n_data;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//