#include "read_fastx_lz4_ATCG_only.hpp"
#include <cstring>
#include <stdexcept>

// =========================================================
// Constructor
// =========================================================
read_fastx_lz4_ATCG_only::read_fastx_lz4_ATCG_only(const std::string& filename, const uint64_t buff_size) 
    : raw_buffer_used(0), raw_idx(0), clean_idx(0), clean_count(0), parser_state(0), qual_skip_cnt(0)
{
    // Initialize capacities
    // raw_capacity is small (4KB) for frequent LZ4 reads
    // clean_capacity is large (2MB) to hold sanitized data for the minimizer
    raw_capacity   = 4096;
    clean_capacity = buff_size; 
    
    // Allocate buffers
    raw_buffer   = new char[raw_capacity];
    clean_buffer = new char[clean_capacity];

    // Open the file
    stream = fopen(filename.c_str(), "r");
    if (!stream) {
        throw std::runtime_error("(EE) File does not exist: " + filename);
    }

    LZ4F_errorCode_t ret = LZ4F_readOpen(&lz4fRead, stream);
    if (LZ4F_isError(ret)) {
        printf("(EE) LZ4F_readOpen error: %s\n", LZ4F_getErrorName(ret));
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }
}

// =========================================================
// Destructor
// =========================================================
read_fastx_lz4_ATCG_only::~read_fastx_lz4_ATCG_only() {
    if (raw_buffer) delete[] raw_buffer;
    if (clean_buffer) delete[] clean_buffer;
    
    LZ4F_readClose(lz4fRead);
    if (stream) fclose(stream);
}


// =========================================================
// Load Next Chunk (The Engine)
// =========================================================
std::tuple<bool, bool> read_fastx_lz4_ATCG_only::load_next_chunk(char** out_ptr, uint64_t* out_size) {
    bool found_eof = false;
    bool found_end_of_seq = false; 
    
    clean_idx = 0;

    // -----------------------------------------------------
    // STEP 1: FILL LOOP
    // -----------------------------------------------------
    // Fill clean_buffer until it is almost full. 
    // We leave a 4096 byte margin to safely flush the last raw buffer.
    while (clean_idx < clean_capacity - 4096) { 
        
        // 2a. REFILL RAW BUFFER
        // If we exhausted the raw buffer (or just started), read more from GZIP.
        if (raw_idx == raw_buffer_used || raw_idx == 0) { 
            raw_buffer_used = LZ4F_read (lz4fRead, raw_buffer, raw_capacity * sizeof(char) );
            raw_idx = 0;

            if (raw_buffer_used == 0) {
                found_eof = true;
                break; // Stop filling if file ends
            }
        }
        
        // -------------------------------------------------
        // STEP 2: SANITIZATION (State Machine)
        // -------------------------------------------------
        // Parse raw bytes into clean ATCG sequences.
        /* STATE DEFINITIONS:
               0: HEADER (Skip everything until newline)
               1: SEQ    (Read ATCG, look for newline or +)
               2: PLUS   (Found '+', skip rest of line)
               3: QUAL   (Skip as many chars as we read in SEQ)
        */

        while (raw_idx < raw_buffer_used) {
            char c = raw_buffer[raw_idx++];

            if (parser_state == 0) { // HEADER (@ or >)
                // Consume chars until newline sets us to SEQUENCE mode
                if (c == '\n') {
                    parser_state = 1; 
                    qual_skip_cnt = 0;
                }
            }
            else if (parser_state == 1) { // SEQUENCE (ATCG)
                if (c == '\n') continue; // Ignore newlines inside sequence

                if (c == '+') { // FASTQ separator found
                    parser_state = 2; // Move to PLUS state
                    clean_count = clean_idx;
                    *out_ptr  = clean_buffer;
                    *out_size = clean_count;
                    return std::make_tuple(false, true); // Signal End of Sequence
                } 
                else if (c == '>') { // FASTA separator found
                    parser_state = 0; // Move to HEADER state for next seq
                    clean_count = clean_idx;
                    *out_ptr  = clean_buffer;
                    *out_size = clean_count;
                    return std::make_tuple(false, true); // Signal End of Sequence
                }
                else {
                    char upper = c & 0xDF; // Fast Uppercase conversion
                    
                    if (upper == 'A' || upper == 'C' || upper == 'G' || upper == 'T') {
                        clean_buffer[clean_idx++] = upper;
                        qual_skip_cnt++; // Count bases to know how much QUAL to skip
                    }
                    else if (upper == 'U') {
                        clean_buffer[clean_idx++] = 'T'; // Normalize RNA to DNA
                        qual_skip_cnt++;
                    }
                    else { 
                        // Non-nucleotide found (e.g. 'N'). 
                        // Treat as End of Sequence to break the k-mer stream.
                        qual_skip_cnt++;
                        clean_count = clean_idx;
                        *out_ptr  = clean_buffer;
                        *out_size = clean_count;
                        return std::make_tuple(false, true);
                    }
                }
            }
            else if (parser_state == 2) { // PLUS LINE (+)
                if (c == '\n') {
                    parser_state = 3; // End of plus line, start skipping Quality
                }
            }
            else if (parser_state == 3) { // QUALITY SCORES
                if (c == '\n') continue;
                // Skip exactly as many quality chars as we read sequence bases
                if (qual_skip_cnt > 0) qual_skip_cnt--;
                
                // Once done skipping, go back to HEADER for next read
                if (qual_skip_cnt == 0) parser_state = 0;
            }
        }
    }

    // Finalize chunk
    clean_count = clean_idx;
    *out_ptr  = clean_buffer;
    *out_size = clean_count;
    
    // Return tuple: <found_eof, found_end_of_seq>
    return std::make_tuple(found_eof, found_end_of_seq);
}