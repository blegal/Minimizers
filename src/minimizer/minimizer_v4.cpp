
#include "../kmer_list/smer_deduplication.hpp"

#include "../front/fastx/read_fastx_ATCG_only.hpp"
#include "../front/fastx_gz/read_fastx_gz_ATCG_only.hpp"
#include "../front/fastx_bz2/read_fastx_bz2_ATCG_only.hpp"
#include "../front/fastx_lz4/read_fastx_lz4_ATCG_only.hpp"
#include "../front/count_file_lines.hpp"

#include "../hash/CustomMurmurHash3.hpp"
#include "../back/txt/SaveMiniToTxtFile.hpp"
#include "../back/raw/SaveRawToFile.hpp"

#include "../sorting/std_2cores/std_2cores.hpp"
#include "../sorting/std_4cores/std_4cores.hpp"
#include "../sorting/crumsort_2cores/crumsort_2cores.hpp"
#include "../merger/in_file/merger_level_0.hpp"

#define MEM_UNIT 64
#define _debug_ 0
#define _murmurhash_

inline uint64_t mask_right(const uint64_t numbits)
{
    uint64_t mask = -(numbits >= MEM_UNIT) | ((1ULL << numbits) - 1ULL);
    return mask;
}


void minimizer_processing_v4(
        const std::string& i_file    = "none",
        const std::string& o_file    = "none",
        const std::string& algo      = "crumsort",
        const uint64_t  ram_limit_in_MB   = 1024,
        const bool      file_save_output  = true,
        const bool      file_save_debug   = false,
        const uint64_t  kmer = 31,
        const uint64_t  mmer = 19
)
{
    // =========================================================================
    // 1. SETUP & ALLOCATION
    // =========================================================================
    uint64_t buff_size = 2 * 1024 * 1024; // 2MB buffer for reading sequences
    uint64_t max_in_ram = 1024 * 1024 * (uint64_t)ram_limit_in_MB / sizeof(uint64_t);
    uint64_t z = kmer - mmer; // Window size for minimizer selection
    uint64_t mask = mask_right(2 * mmer); 

    // Output Buffer (stores minimizers before flushing/saving)
    std::vector<uint64_t> liste_mini(max_in_ram);
    uint64_t n_minizer = 0;
    
    // List of temporary files created during RAM flush
    std::vector<std::string> file_list;

    // =========================================================================
    // 2. READER INITIALIZATION
    // =========================================================================
    // We request an overlap of (kmer - 1) to ensure no k-mer is lost 
    // when a sequence spans across two file chunks.
    file_reader_ATCG_only* reader;

    if (i_file.substr(i_file.find_last_of(".") + 1) == "bz2")
    {
        reader = new read_fastx_bz2_ATCG_only(i_file, buff_size);
    }
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "gz")
    {
        reader = new read_fastx_gz_ATCG_only(i_file, buff_size);
    }
    else if (i_file.substr(i_file.find_last_of(".") + 1) == "lz4")
    {
        reader = new read_fastx_lz4_ATCG_only(i_file, buff_size);
    }
    else if(
            (i_file.substr(i_file.find_last_of(".") + 1) == "fastx") ||
            (i_file.substr(i_file.find_last_of(".") + 1) == "fasta") ||
            (i_file.substr(i_file.find_last_of(".") + 1) == "fastq") ||
            (i_file.substr(i_file.find_last_of(".") + 1) == "fna")
            )
    {
        reader = new read_fastx_ATCG_only(i_file, buff_size);
    }
    else
    {
        printf("(EE) File extension is not supported (%s)\n", i_file.c_str());
        printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
        exit( EXIT_FAILURE );
    }
    
    char*    seq_buffer = nullptr;
    uint64_t seq_size   = 0;

    bool eof_and_finished = false;

    // =========================================================================
    // 3. MAIN PROCESSING LOOP
    // =========================================================================
    while( true ) {

        // Check if the previous iteration marked global EOF
        if (eof_and_finished) {
            break;
        }

        // Load the initial chunk for this sequence
        // Returns tuple: <End of File (EOF), End of Sequence (EOS)>
        std::tuple<bool, bool> tuple_eof_eos = reader->load_next_chunk(&seq_buffer, &seq_size); 

        // ---------------------------------------------------------------------
        // 3.1. SKIP TINY SEQS
        // ---------------------------------------------------------------------
        // If the chunk is smaller than kmer, we can't compute any minimizers.
        // We loop until we get a valid chunk or hit EOF.
        while (seq_size < kmer){
            if (std::get<0>(tuple_eof_eos) == true) { // EOF reached
                eof_and_finished = true;
                break;
            }
            // Sequence ended (EOS), but was too short. Try next seq.
            tuple_eof_eos = reader->load_next_chunk(&seq_buffer, &seq_size);
        }

        if (eof_and_finished) {
            break;
        }

        // ---------------------------------------------------------------------
        // 3.2. INITIALIZE ROLLING HASH (Phase 1: First M-mer)
        // ---------------------------------------------------------------------
        // Prepare the sliding window buffer
        uint64_t hash_window[z + 1]; 
        for(uint64_t x = 0; x <= z; x += 1)
            hash_window[x] = UINT64_MAX;
        
        uint64_t current_mmer = 0;
        uint64_t cur_inv_mmer = 0;
        uint64_t cnt = 0;

        // Prepare the very first m-mer (first 18 bases if m=19)
        for(uint64_t x = 0; x < mmer - 1; x += 1)
        {
            const uint64_t encoded = ((seq_buffer[cnt] >> 1) & 0b11);
            current_mmer <<= 2;
            current_mmer |= encoded; // ASCII => 2-bit encoding
            current_mmer &= mask;
            cur_inv_mmer >>= 2;
            cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); 
            cnt          += 1;
        }

        // ---------------------------------------------------------------------
        // 3.3. FILL HASH WINDOW (Phase 2: First K-mer)
        // ---------------------------------------------------------------------
        // Compute hashes for the first 'z' m-mers to fill the window and find the first minimizer
        uint64_t minv = UINT64_MAX;
        for(uint64_t m_pos = 0; m_pos <= z; m_pos += 1)
        {
            const uint64_t encoded = ((seq_buffer[cnt] >> 1) & 0b11); 
            current_mmer <<= 2;
            current_mmer |= encoded;
            current_mmer &= mask;
            cur_inv_mmer >>= 2;
            cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); 
            
            const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;

            uint64_t tab[2];
            CustomMurmurHash3_x64_128<8> ( &canon, 42, tab );

            const uint64_t s_hash = tab[0];
            hash_window[m_pos]    = s_hash; // Store hash in window
            minv                  = (s_hash < minv) ? s_hash : minv;
            cnt                   += 1; 
        }

        // Store the first minimizer found
        if( n_minizer == 0 ){
            liste_mini[n_minizer++] = minv;
        }else if( liste_mini[n_minizer-1] != minv ){
            liste_mini[n_minizer++] = minv;
        }

        // ---------------------------------------------------------------------
        // 3.4. ROLLING HASH LOOP (Phase 3: All other K-mers)
        // ---------------------------------------------------------------------
        while ( true ) { // Loop for the rest of the sequence (across chunks)
            
            // Process all remaining bases in the CURRENT buffer
            while (cnt < seq_size) { 
                const uint64_t encoded = ((seq_buffer[cnt] >> 1) & 0b11); 
                current_mmer <<= 2;                                     
                current_mmer |= encoded;
                current_mmer &= mask;
                cur_inv_mmer >>= 2;
                cur_inv_mmer |= ( (0x2 ^ encoded) << (2 * (mmer - 1))); 

                const uint64_t canon  = (current_mmer < cur_inv_mmer) ? current_mmer : cur_inv_mmer;

                uint64_t tab[2];
                CustomMurmurHash3_x64_128<8> ( &canon, 42, tab );

                const uint64_t s_hash = tab[0];
                minv                  = (s_hash < minv) ? s_hash : minv;

                // Update Window
                if( minv == hash_window[0] ) // Outgoing m-mer was the minimum; rescan window
                {
                    minv = s_hash;
                    for(uint64_t p = 0; p < z; p += 1) {
                        const uint64_t value = hash_window[p + 1];
                        minv = (minv < value) ? minv : value;
                        hash_window[p] = value;
                    }
                    hash_window[z] = s_hash; 
                }else{ // Standard shift
                    for(uint64_t p = 0; p < z; p += 1) {
                        hash_window[p] = hash_window[p+1]; 
                    }
                    hash_window[z] = s_hash; 
                }

                // Store Minimizer (if new)
                if( liste_mini[n_minizer-1] != minv ){
                    liste_mini[n_minizer++] = minv;

                    // Handle RAM overflow
                    if( n_minizer >= (max_in_ram - 2) )
                    {
                        std::string t_file = o_file + "." + std::to_string( file_list.size() );
                        
                        // Sort and deduplicate in RAM before flushing
                        crumsort_prim( liste_mini.data(), n_minizer - 1, 9 /*uint64*/ );
                        uint64_t n_elements = smer_deduplication(liste_mini, n_minizer - 1);

                        SaveRawToFile(t_file, liste_mini, n_elements - 1);

                        file_list.push_back( t_file );
                        liste_mini[0] = liste_mini[n_minizer-1]; // Keep last min for continuity
                        n_minizer     = 1;
                    }
                }

                cnt += 1; 
            }

            // -----------------------------------------------------------------
            // 3.5. HANDLE CHUNK BOUNDARIES
            // -----------------------------------------------------------------
            
            // If EOF reached, signal to break outer loop
            if ( std::get<0>(tuple_eof_eos) == true ) {
                eof_and_finished = true; 
                break;
            }

            // If End of Sequence (EOS) reached, prepare for next sequence
            if ( std::get<1>(tuple_eof_eos) == true ) {
                break; // Break inner loop to restart Phase 1 for new seq
            }

            // Load NEXT chunk for the SAME sequence
            tuple_eof_eos = reader->load_next_chunk(&seq_buffer, &seq_size);
            
            // Set 'cnt' to skip the overlap we already processed (first k-1 bases)
            cnt = 0; 
        }
    }


    // =========================================================================
    // 4. FINALIZATION & SAVING
    // =========================================================================
    
    // CASE A: Temporary files exist (RAM limit was exceeded)
    if( file_list.size() != 0 )
    {
        std::string t_file = o_file + "." + std::to_string( file_list.size() );
        crumsort_prim( liste_mini.data(), n_minizer, 9 );

        uint64_t n_elements = smer_deduplication(liste_mini, n_minizer);
        SaveRawToFile(t_file, liste_mini, n_elements);

        file_list.push_back( t_file );

        // Merge all temporary files
        uint64_t name_c = file_list.size();
        while(file_list.size() > 1)
        {
            const std::string t_file = o_file + "." + std::to_string( name_c++ );
            merge_level_0(file_list[0], file_list[1], t_file);
            std::remove(file_list[0].c_str());
            std::remove(file_list[1].c_str());
            file_list.erase(file_list.begin());
            file_list.erase(file_list.begin());
            file_list.push_back( t_file );
        }
        std::rename(file_list[file_list.size()-1].c_str(), o_file.c_str());
        return;
    }

    // CASE B: Everything fit in RAM (Direct Save)
    liste_mini.resize(n_minizer);

    if( file_save_debug ){
        SaveMiniToTxtFile_v2(o_file + ".non-sorted-v2.txt", liste_mini);
    }

    // Sorting Selection
    if( algo == "std::sort" ) {
        std::sort( liste_mini.begin(), liste_mini.end() );
    } else if( algo == "std_2cores" ) {
        std_2cores( liste_mini );
    } else if( algo == "std_4cores" ) {
        std_4cores( liste_mini );
    } else if( algo == "crumsort" ) {
        crumsort_prim( liste_mini.data(), liste_mini.size(), 9 );
    } else if( algo == "crumsort_2cores" ) {
        crumsort_2cores( liste_mini );
    } else {
        printf("(EE) Sorting algorithm is invalid (%s)\n", algo.c_str());
        exit( EXIT_FAILURE );
    }

    // Final Deduplication
    smer_deduplication( liste_mini );

    if( file_save_debug ){
        SaveMiniToTxtFile_v2(o_file + ".txt", liste_mini);
    }

    if( file_save_output ){
        SaveRawToFile(o_file, liste_mini);
    }

    delete reader;
}