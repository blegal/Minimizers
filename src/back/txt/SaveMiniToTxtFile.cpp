#include "./SaveMiniToTxtFile.hpp"
#include "../../progress/progressbar.h"

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

bool SaveMiniToTxtFile(const std::string filename, const std::vector<uint64_t> list_hash)
{
    printf("(II)\n");
    printf("(II) Saving minimizer data set in [%s]\n", filename.c_str());
    progressbar *progress = progressbar_new("Saving minimizer values (TXT)", 100);
    double start_time = omp_get_wtime();

    std::string n_file = filename;
    FILE* f = fopen( n_file.c_str(), "w" );
    if( f == NULL )
    {
        printf("(EE) File does not exist !\n");
        exit( EXIT_FAILURE );
    }

    const int n_lines = list_hash.size();
    const int prog_step = n_lines / 100;

    for(int y = 0; y < n_lines; y += 1)
    {
        fprintf(f, "%16.16llX\n", list_hash[y]);

        if( y%prog_step == 0)
            progressbar_inc(progress);
    }

    fclose( f );
    double end_time = omp_get_wtime();

    progressbar_finish(progress);

    printf("(II) - Execution time    = %f\n", end_time - start_time);
    return true;
}

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

inline char to_ascii(const int64_t value)
{
    const int car = value & 0x0F;
    const int off = ( car < 10 ) ? '0' : ('A' - 10);
    const int res = car + off;
//  printf("%X => %c\n", car, res);
    return res;
}

bool SaveMiniToTxtFile_v2(const std::string filename, const std::vector<uint64_t> list_hash)
{
    printf("(II)\n");
    printf("(II) Saving minimizer data set in [%s]\n", filename.c_str());
    const int n_lines = list_hash.size();
    progressbar *progress = progressbar_new("Saving minimizer values (TXT)", 100);
    double start_time = omp_get_wtime();

    std::string n_file = filename;
    FILE* f = fopen( n_file.c_str(), "w" );
    if( f == NULL )
    {
        printf("(EE) File does not exist (%s))\n", filename.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    const int prog_step = n_lines / 100;
    const int buffer_size = 64 * 1024 * 1024;
    char* tampon = new char[ buffer_size ];

    int cnt = 0;

    for(int y = 0; y < n_lines; y += 1)
    {
        const uint64_t value = list_hash[y];
        char* buffer = tampon + cnt;
        buffer[ 0] = to_ascii(value >> 60);
        buffer[ 1] = to_ascii(value >> 56);
        buffer[ 2] = to_ascii(value >> 52);
        buffer[ 3] = to_ascii(value >> 48);
        buffer[ 4] = to_ascii(value >> 44);
        buffer[ 5] = to_ascii(value >> 40);
        buffer[ 6] = to_ascii(value >> 36);
        buffer[ 7] = to_ascii(value >> 32);
        buffer[ 8] = to_ascii(value >> 28);
        buffer[ 9] = to_ascii(value >> 24);
        buffer[10] = to_ascii(value >> 20);
        buffer[11] = to_ascii(value >> 16);
        buffer[12] = to_ascii(value >> 12);
        buffer[13] = to_ascii(value >>  8);
        buffer[14] = to_ascii(value >>  4);
        buffer[15] = to_ascii( value           );
        buffer[16] =  '\n';
        cnt       += 17;

        if( cnt >= (buffer_size - 1024) )
        {
            fwrite(tampon, sizeof(char), cnt, f);
            cnt = 0;
        }

        if( y%prog_step == 0)
            progressbar_inc(progress);
    }

    //
    // On flushe la fin du buffer de données avant de fermer le fichier
    //

    if( cnt != 0 )
    {
        fwrite(tampon, sizeof(char), cnt, f);
    }

    fclose( f );
    double end_time = omp_get_wtime();

    progressbar_finish(progress);

    printf("(II) - Execution time    = %f\n", end_time - start_time);
    return true;
}
