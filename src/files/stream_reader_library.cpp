#include "stream_reader_library.hpp"
#include "bz2/reader/stream_bz2_reader.hpp"
#include "gz/reader/stream_gz_reader.hpp"
#include "lz4/reader/stream_lz4_reader.hpp"
#include "raw/reader/stream_raw_reader.hpp"

stream_reader* stream_reader_library::allocate(const std::string& i_file)
{
    //
    // Allocating the object that performs fast file parsing
    //
    stream_reader* reader;
    if (i_file.substr(i_file.find_last_of(".") + 1) == "bz2")
    {
        reader = new stream_bz2_reader(i_file);
    }
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "gz")
    {
        reader = new stream_gz_reader(i_file);
    }
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "lz4")
    {
        reader = new stream_lz4_reader(i_file);
    }
    else
    {
        reader = new stream_raw_reader(i_file);
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
