#include "stream_gz_writer.hpp"
#include "../../../tools/colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
stream_gz_writer::stream_gz_writer(const std::string& filen)
{
    //
    //   Open the gzip (.gz) file at path for reading and decompressing, or
    //   compressing and writing.  The mode parameter is as in fopen ("rb" or "wb")
    //   but can also include a compression level ("wb9") or a strategy: 'f' for
    //   filtered data as in "wb6f", 'h' for Huffman-only compression as in "wb1h",
    //   'R' for run-length encoding as in "wb1R", or 'F' for fixed code compression
    //   as in "wb9F".  (See the description of deflateInit2 for more information
    //   about the strategy parameter.)  'T' will request transparent writing or
    //   appending with no compression and not using the gzip format.
    //
    gzfp = gzopen(filen.c_str(), "wb");
    if( gzfp == NULL )
    {
        error_section();
        printf("(EE) It is impossible to create the file (%s))\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }
    is_fopen = true;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
stream_gz_writer::~stream_gz_writer()
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
bool stream_gz_writer::is_open ()
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
int stream_gz_writer::write(void* buffer, int eSize, int eCount)
{
    int nwrite = gzwrite( gzfp, buffer, eSize * eCount );
    if( nwrite == 0 )
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
    return nwrite;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void stream_gz_writer::close()
{
    gzclose( gzfp );
    is_fopen = false;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//