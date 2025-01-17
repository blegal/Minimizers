#include <cstdio>
#include <vector>
#include <omp.h>
#include <getopt.h>
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void GenTestFile(std::string file, const uint64_t value, const int length)
{
    FILE* f = fopen(file.c_str(), "w");

    uint64_t data = 0;
    fwrite(&data, sizeof(uint64_t), 1, f);

    for(int x = 0; x < length; x += 1)
    {
        data = (value << (8 * x));
        fwrite(&data, sizeof(uint64_t), 1, f);
    }
    fclose( f );
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int main(int argc, char* argv[])
{
    //
    const int size = 8;
    //
    GenTestFile("../data/test_files_16/testf_0", 0x00, size);
    GenTestFile("../data/test_files_16/testf_1", 0x11, size);
    GenTestFile("../data/test_files_16/testf_2", 0x22, size);
    GenTestFile("../data/test_files_16/testf_3", 0x33, size);
    GenTestFile("../data/test_files_16/testf_4", 0x44, size);
    GenTestFile("../data/test_files_16/testf_5", 0x55, size);
    GenTestFile("../data/test_files_16/testf_6", 0x66, size);
    GenTestFile("../data/test_files_16/testf_7", 0x77, size);
    GenTestFile("../data/test_files_16/testf_8", 0x88, size);
    GenTestFile("../data/test_files_16/testf_9", 0x99, size);
    GenTestFile("../data/test_files_16/testf_a", 0xAA, size);
    GenTestFile("../data/test_files_16/testf_b", 0xBB, size);
    GenTestFile("../data/test_files_16/testf_c", 0xCC, size);
    GenTestFile("../data/test_files_16/testf_d", 0xDD, size);
    GenTestFile("../data/test_files_16/testf_e", 0xEE, size);
    GenTestFile("../data/test_files_16/testf_f", 0xFF, size);
    //
    //
    //
    for (int i = 0; i < 256; i += 1) {
        std::string fname = "../data/test_files_256/testf_";
        fname += (i < 100) ? "0": "";
        fname += (i <  10) ? "0": "";
        fname += std::to_string(i);
        GenTestFile(fname, i, size);
    }
    //
    //
    //
    return EXIT_SUCCESS;
}
