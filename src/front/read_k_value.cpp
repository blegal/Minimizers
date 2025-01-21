#include "./read_k_value.hpp"
#include "./fastx/read_fastx_k_value.hpp"
#include "./fastx_bz2/read_fastx_bz2_k_value.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int read_k_value(std::string filen)
{
    if (filen.substr(filen.find_last_of(".") + 1) == "bz2")
    {
        return read_fastx_bz2_k_value(filen);
    }
    else if (filen.substr(filen.find_last_of(".") + 1) == "fastx")
    {
        return read_fastx_k_value(filen);
    }else{
        printf("(EE) File extension is not supported (%s)\n", filen.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
        return -1;
    }
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
