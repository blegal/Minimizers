#include "SaveRawToFile.hpp"
#include "../../files/stream_writer_library.hpp"

//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool SaveRawToFile(const std::string filename, const std::vector<uint64_t> list_hash, const int n_elements)
{
#if 1
    stream_writer* o_file = stream_writer_library::allocate( filename );
    o_file->write((void*)list_hash.data(), sizeof(uint64_t), n_elements);
    o_file->close();
    delete o_file;
#else
    std::string n_file = filename;
    FILE* f = fopen( n_file.c_str(), "w" );
    if( f == NULL )
    {
        printf("(EE) File does not exist (%s))\n", filename.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }

    fwrite(list_hash.data(), sizeof(uint64_t), n_elements, f);

    fclose( f );
#endif
    return true;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
    bool SaveRawToFile(const std::string filename, const std::vector<uint64_t> list_hash)
    {
        SaveRawToFile(filename, list_hash, list_hash.size());
        return true;
    }
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//