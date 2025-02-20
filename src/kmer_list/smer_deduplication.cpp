#include "smer_deduplication.hpp"
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
void smer_deduplication(std::vector<uint64_t>& values)
{
    uint64_t* ptr_i = values.data() + 1;
    uint64_t* ptr_o = values.data() + 1;
    int64_t length  = values.size();

    uint64_t value = values[0];
    for(int64_t x = 1; x < length; x += 1)
    {
        if( value != *ptr_i )
        {
            value  = *ptr_i;
            *ptr_o = value;
            ptr_o += 1;
        }
        ptr_i += 1;
    }
    int64_t new_length  = ptr_o - values.data();
    values.resize( new_length );
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
int smer_deduplication(std::vector<uint64_t>& values, const int n_elements)
{
    uint64_t* ptr_i = values.data() + 1;
    uint64_t* ptr_o = values.data() + 1;

    uint64_t value = values[0];
    for(int64_t x = 1; x < n_elements; x += 1)
    {
        if( value != *ptr_i )
        {
            value  = *ptr_i;
            *ptr_o = value;
            ptr_o += 1;
        }
        ptr_i += 1;
    }
    int64_t new_length  = ptr_o - values.data();
    return new_length;
}
//
//
//
////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
