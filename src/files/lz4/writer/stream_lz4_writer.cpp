#include "stream_lz4_writer.hpp"
#include "../../../tools/colors.hpp"
#include "../../../front/fastx_lz4/lz4/lz4file.h"
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
stream_lz4_writer::stream_lz4_writer(const std::string& filen)
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
    const LZ4F_errorCode_t ret = LZ4F_writeOpen(&lz4fWrite, stream, NULL);
    if (LZ4F_isError(ret)) {
        error_section();
        printf("LZ4F_writeOpen error: %s\n", LZ4F_getErrorName(ret));
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
stream_lz4_writer::~stream_lz4_writer()
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
bool stream_lz4_writer::is_open ()
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
int stream_lz4_writer::write(void* buffer, int eSize, int eCount)
{
    const LZ4F_errorCode_t ret = LZ4F_write(lz4fWrite, buffer, eSize * eCount);
    if (LZ4F_isError(ret)) {
        error_section();
        printf("(EE) An error occured during the LZ4F_write task:\n");
        printf("(EE) - Amount of data to write (eSize)  : %d\n", eSize);
        printf("(EE) - Amount of data to write (eCount) : %d\n", eCount);
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        reset_section();
        exit( EXIT_FAILURE );
    }
    if( ret != (eSize * eCount) ){
        warning_section();
        printf("(WW) La fonction LZ4F_write n'a pas compressé toutes les données:\n");
        printf("(WW) - Amount of data wrote    (ret)    : %d\n", ret);
        printf("(WW) - Amount of data to write (eSize)  : %d\n", eSize);
        printf("(WW) - Amount of data to write (eCount) : %d\n", eCount);
        printf("(WW) Error location : %s %d\n", __FILE__, __LINE__);
        printf("(WW) Ce cas de figure n'est pas géré pour le moment => STOP\n");
        reset_section();
        exit( EXIT_FAILURE );
    }
    return (ret / eSize); // nombre d'éléments de taille eSize
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void stream_lz4_writer::close()
{
    const LZ4F_errorCode_t ret = LZ4F_writeClose(lz4fWrite);
    if (LZ4F_isError( ret )) {
        error_section();
        printf("(EE) LZ4F_writeClose: %s\n", LZ4F_getErrorName(ret));
        reset_section();
        exit( EXIT_FAILURE );
    }
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