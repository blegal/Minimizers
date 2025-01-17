#include <cstdio>
#include <fstream>
#include <vector>
#include <omp.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>

#include "./merger/merger_level_0.hpp"
#include "./merger/merger_level_1.hpp"
#include "./merger/merger_level_2_32.hpp"

#include "./merger/merger_in.hpp"

uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}

std::vector<std::string> file_list()
{
    std::vector<std::string> filenames;
    DIR* dpdf = opendir("/data/files");
    struct dirent *epdf;
    if (dpdf != NULL) {
        while( (epdf = readdir(dpdf)) ){
            filenames.push_back( std::string(epdf->d_name) );
        }
    }
    return filenames;
}

std::vector<std::string> file_list_cpp()
{
    std::vector<std::string> paths;
    std::string path = "Assets/";
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        paths.push_back(entry.path().string());
    }
    return paths;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

void gen_tests()
{
    const int length = 16;

    //
    //
    //
    FILE* f = fopen("test_a.raw",   "w");
    for(int x = 0; x < length; x += 1)
    {
        const uint64_t value = 2 * x;
        fwrite(&value, sizeof(uint64_t), 1, f);
    }
    fclose( f );

    //
    //
    //
    f = fopen("test_b.raw",   "w");
    for(int x = 0; x < length; x += 1)
    {
        const uint64_t value = 2 * x + 1;
        fwrite(&value, sizeof(uint64_t), 1, f);
    }
    fclose( f );

    //
    //
    //
    f = fopen("test_c.raw",   "w");
    for(int x = 0; x < length; x += 1)
    {
        const uint64_t value = 2 * length + 2 * x;
        fwrite(&value, sizeof(uint64_t), 1, f);
    }
    fclose( f );

    //
    //
    //
    f = fopen("test_d.raw",   "w");
    for(int x = 0; x < length; x += 1)
    {
        const uint64_t value = 2 * length + 2 * x + 1;
        fwrite(&value, sizeof(uint64_t), 1, f);
    }
    fclose( f );

}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        gen_tests();
        exit( EXIT_FAILURE );
    }

    std::string ifile_1 = argv[1];
    std::string ifile_2 = argv[2];
    std::string o_file  = argv[3];

    int level = 0;
    if (argc >= 5) {
        level = std::atoi( argv[4] );
    }

    /*
     * Counting the number of SMER in the file (to allocate memory)
     */

    const uint64_t size_1_bytes = get_file_size(ifile_1);
    const uint64_t size_2_bytes = get_file_size(ifile_2);

    const uint64_t size_1_Mbytes = size_1_bytes / 1024 / 1024;
    const uint64_t size_2_Mbytes = size_2_bytes / 1024 / 1024;

    const uint64_t mizer_1 = size_1_bytes / sizeof(uint64_t);
    const uint64_t mizer_2 = size_2_bytes / sizeof(uint64_t);

    printf("(II)\n");
    printf("(II) Document (1) information\n");
    printf("(II) - filename    : %s\n", ifile_1.c_str());
    printf("(II) - filesize    : %llu bytes\n", size_1_bytes);
    printf("(II) -             : %llu Mbytes\n", size_1_Mbytes);
    printf("(II) - #minimizers : %llu elements\n", mizer_1);
    printf("(II)\n");
    printf("(II) Document (2) information\n");
    printf("(II) - filename    : %s\n", ifile_2.c_str());
    printf("(II) - filesize    : %llu bytes\n", size_2_bytes);
    printf("(II) -             : %llu Mbytes\n", size_2_Mbytes);
    printf("(II) - #minimizers : %llu elements\n", mizer_2);
    printf("(II)\n");
    printf("(II) #colors ifile : %d\n",     level);
    printf("(II) #colors ofile : %d\n", 2 * level);
    printf("(II)\n");

    double start_time = omp_get_wtime();

    merger_in(ifile_1, ifile_2, o_file, level);

    double end_time = omp_get_wtime();

    const uint64_t size_o_bytes  = get_file_size(o_file);
    const uint64_t size_o_Mbytes = size_o_bytes / 1024 / 1024;
    const uint64_t mizer_o       = size_o_bytes / sizeof(uint64_t);

    const uint64_t start   = mizer_1 + mizer_2;
    const uint64_t skipped = start - mizer_o;

    printf("(II) Document (3) information\n");
    printf("(II) - filename      : %s\n", o_file.c_str());
    printf("(II) - filesize      : %llu bytes\n",  size_o_bytes);
    printf("(II) -               : %llu Mbytes\n", size_o_Mbytes);
    printf("(II) - #minzer start : %llu elements\n", start);
    printf("(II) -       skipped : %llu elements\n", skipped);
    printf("(II) -         final : %llu elements\n", mizer_o);
    printf("(II)\n");
    printf("(II) - Exec. time    : %1.2f sec.\n", end_time - start_time);
    printf("(II)\n");

    return 0;
}