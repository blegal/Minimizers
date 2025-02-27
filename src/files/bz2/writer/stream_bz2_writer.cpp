#include "stream_bz2_writer.hpp"
#include "../../../tools/colors.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

/*! LZ4F_preferences_t :
 *  makes it possible to supply advanced compression instructions to streaming interface.
 *  Structure must be first init to 0, using memset() or LZ4F_INIT_PREFERENCES,
 *  setting all parameters to default.
 *  All reserved fields must be set to zero. */
//typedef struct {
//    LZ4F_frameInfo_t frameInfo;
//    int      compressionLevel;    /* 0: default (fast mode); values > LZ4HC_CLEVEL_MAX count as LZ4HC_CLEVEL_MAX; values < 0 trigger "fast acceleration" */
//    unsigned autoFlush;           /* 1: always flush; reduces usage of internal buffers */
//    unsigned favorDecSpeed;       /* 1: parser favors decompression speed vs compression ratio. Only works for high compression modes (>= LZ4HC_CLEVEL_OPT_MIN) */  /* v1.8.2+ */
//    unsigned reserved[3];         /* must be zero for forward compatibility */
//} LZ4F_preferences_t;
stream_bz2_writer::stream_bz2_writer(const std::string& filen)
{
    //
    // Ouverture du fichier en mode écriture !
    //
    stream = fopen( filen.c_str(), "wb" );
    if( stream == NULL )
    {
        error_section();
        printf("(EE) It is impossible to create the file (%s))\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }

    //
    // Ouverture du flux compréssé avec les parametres par défaut
    //
    int bzerror;
    // https://sourceware.org/pub/bzip2/docs/v101/manual_3.html
    bz2fWrite = BZ2_bzWriteOpen ( &bzerror, stream, 9, 0, 30 ); // 30 = parametre par défaut
    if (bzerror != BZ_OK) {
        error_section();
        printf("(EE) BZ2_bzWriteOpen error\n");
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
stream_bz2_writer::~stream_bz2_writer()
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
bool stream_bz2_writer::is_open ()
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
int stream_bz2_writer::write(void* buffer, int eSize, int eCount)
{
    int bzerror;
    int nwrite = eSize * eCount;
    BZ2_bzWrite ( &bzerror, bz2fWrite, buffer, nwrite );
    if (bzerror != BZ_OK) {
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
void stream_bz2_writer::close()
{
    int bzerror;
    unsigned int nbytes_in;
    unsigned int nbytes_out;
    BZ2_bzWriteClose ( &bzerror, bz2fWrite, 0, &nbytes_in, &nbytes_out );
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