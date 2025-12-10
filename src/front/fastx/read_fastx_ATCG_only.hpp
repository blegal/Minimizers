#pragma once
#include "../file_reader_ATCG_only.hpp"

class read_fastx_ATCG_only : public file_reader_ATCG_only
{
    // Buffers
    char* raw_buffer;      // Raw data from file
    char* clean_buffer;    // Sanitized ATCG data
    uint64_t raw_buffer_used;      
    uint64_t raw_capacity;
    uint64_t clean_capacity;
    uint64_t raw_idx;
    uint64_t clean_idx;

    
    // State
    FILE* stream;
    uint64_t overlap;         // Size of k-mer - 1
    uint64_t clean_count;     // Valid bytes in clean_buffer
    
    // Parsing State Machine
    uint64_t parser_state;    // 0=HEADER, 1=SEQ, 2=PLUS, 3=QUAL
    uint64_t qual_skip_cnt;   // How many quality chars to skip

public:
    // Constructor now takes 'overlap' (k-1) to handle k-mer stitching
    read_fastx_ATCG_only(const std::string& filename, const uint64_t buff_size);
    ~read_fastx_ATCG_only();

    /*
    // Fills the buffer with the next seq of pure ATCG up to 2MB capacity, needs to be repeatedly called
    // Keep stored state of reading head in input file
    // Keep an overlap of k-1 bases for rolling hash function 
    // Returns <found_eof, found_end_of_seq>
    */
    std::tuple<bool, bool> load_next_chunk(char** out_ptr, uint64_t* out_size);
};