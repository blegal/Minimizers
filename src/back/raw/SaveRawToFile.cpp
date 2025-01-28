#include "SaveRawToFile.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
bool SaveRawToFile(const std::string filename, const std::vector<uint64_t> list_hash, const int n_elements)
{
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