#include "deduplication.hpp"

void VectorDeduplication(std::vector<uint64_t>& values)
{
/*
    printf("(II)\n");
    printf("(II) Launching the simplification step\n");
    printf("(II) - Number of samples (start) = %10ld\n", values.size());
    double start_time = omp_get_wtime();
*/
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
/*
    double end_time = omp_get_wtime();
    printf("(II) - Number of samples (stop)  = %10ld\n", values.size());
    printf("(II) - Execution time    = %f\n", end_time - start_time);
    */
}