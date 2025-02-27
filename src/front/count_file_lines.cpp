#include "./count_file_lines.hpp"
#include "./fastx/count_lines_fastx.hpp"
#include "./fastx_bz2/count_lines_fastx_bz2.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
uint64_t count_file_lines(std::string filen) {
    if (filen.substr(filen.find_last_of(".") + 1) == "bz2")
    {
        return count_lines_fastx_bz2(filen);
    }
    else if (filen.substr(filen.find_last_of(".") + 1) == "fastx")
    {
        return count_lines_fastx(filen);
    }else{
        printf("(EE) File extension is not supported (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
        return -1;
    }
//#if 0
////    printf("(II) Counting the number of sequences (c)\n");
//    FILE* f = fopen( filename.c_str(), "r" );
//    if( f == NULL )
//    {
//        printf("(EE) File does not exist (%s))\n", filename.c_str());
//        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
//        exit( EXIT_FAILURE );
//    }
//
//    uint64_t n_sequences = 0;
//    char buffer[1024 * 1024];
////    double start_time = omp_get_wtime();
//    while ( !feof(f) )
//    {
//        int n = fread(buffer, sizeof(char), 1024 * 1024, f);
//        for(int x = 0; x < n; x += 1)
//            n_sequences += (buffer[x] == '\n');
//    }
//    fclose( f );
////    double end_time = omp_get_wtime();
////    printf("(II) - %llu lines in file\n", n_sequences);
////    printf("(II) - It took %g seconds\n", end_time - start_time);
//    return n_sequences;
//#endif
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
