#include "stream_gz_reader.hpp"
#include "../../../tools/colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
stream_gz_reader::stream_gz_reader(const std::string& filen)
{
    gzfp = gzopen(filen.c_str(), "rb");
    if( gzfp == NULL )
    {
        error_section();
        printf("(EE) It is impossible to open the file (%s))\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
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
stream_gz_reader::~stream_gz_reader()
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
bool stream_gz_reader::is_open ()
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
bool stream_gz_reader::is_eof()
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
int stream_gz_reader::read(char* buffer, int eSize, int eCount)
{
    const int n_data = gzread(gzfp, buffer,  eSize * eCount);
    if( n_data == Z_STREAM_END ) {
        is_foef = true;
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

    is_foef |= ( (eCount * eSize) != n_data ); // a t'on atteint la fin du fichier ?
    return (n_data / eSize); // nombre d'éléments lu et NON PAS le nombre de bytes !
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void stream_gz_reader::close()
{
    gzclose(gzfp);
    is_fopen = false;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//