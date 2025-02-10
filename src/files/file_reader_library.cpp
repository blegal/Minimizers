#include "file_reader_library.hpp"
#include "bz2/reader/file_bz2_reader.hpp"
#include "gz/reader/file_gz_reader.hpp"
#include "lz4/reader/file_lz4_reader.hpp"
#include "raw/reader/file_raw_reader.hpp"

file_reader* file_reader_library::allocate(const std::string& i_file)
{
    //
    // Allocating the object that performs fast file parsing
    //
    file_reader* reader;
    if (i_file.substr(i_file.find_last_of(".") + 1) == "bz2")
    {
        reader = new file_bz2_reader(i_file);
    }
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "gz")
    {
        reader = new file_gz_reader(i_file);
    }
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "lz4")
    {
        reader = new file_lz4_reader(i_file);
    }
    else
    {
        reader = new file_raw_reader(i_file);
    }
/*
    else
    {
        printf("(EE) File extension is not supported (%s)\n", i_file.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }
*/
    return reader;
}
