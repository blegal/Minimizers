#include "file_writer_library.hpp"

#include "raw/writer/file_raw_writer.hpp"
#include "bz2/writer/file_bz2_writer.hpp"
#include "lz4/writer/file_lz4_writer.hpp"
#include "gz/writer/file_gz_writer.hpp"

file_writer* file_writer_library::allocate(const std::string& i_file)
{
    //
    // Allocating the object that performs fast file parsing
    //
    file_writer* writer;
    if (i_file.substr(i_file.find_last_of(".") + 1) == "bz2")
    {
        writer = new file_bz2_writer(i_file);
    }
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "gz")
    {
        writer = new file_gz_writer(i_file);
    }
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "lz4")
    {
        writer = new file_lz4_writer(i_file);
    }
    else{
        writer = new file_raw_writer(i_file);
    }
    /*
    else
    {
        printf("(EE) File extension is not supported (%s)\n", i_file.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }*/

    return writer;
}
