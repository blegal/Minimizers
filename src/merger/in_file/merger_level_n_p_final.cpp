#include "merger_level_1.hpp"
#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"

inline int bits_needed(uint64_t b) {
    if (b == 0) return 1;
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
    // Fast hardware instruction (BSR equivalent)
    return 64 - __builtin_clzll(b);
#else
    // Portable fallback
    int bits = 0;
    do {
        bits++;
        b >>= 1;
    } while (b);
    return bits;
#endif
}

inline int popcount64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
    // Use fast builtin instruction (POPCNT)
    return __builtin_popcountll(x);
#elif defined(_MSC_VER)
    // Use MSVC intrinsic
    return static_cast<int>(__popcnt64(x));
#else
    // Portable fallback (Brian Kernighan’s algorithm)
    int count = 0;
    while (x) {
        x &= (x - 1); // clear the lowest set bit
        count++;
    }
    return count;
#endif
}

void merge_level_n_p_final(
    /* 
        Merges 2 files of different color number
        splitting dense and sparse colors into two output files
    */
        const std::string& ifile_1, //bigger, multiple of 64
        const std::string& ifile_2, // smaller, might not be multiple of 64
        const std::string& o_file,
        const std::string& o_file_sparse,
        const int level_1,
        const int level_2)
{
    std::cerr << "Starting final merge\n";
    const int n_u64_per_cols_1 = (level_1+63) / 64; // En entrée de la fonction
    const int n_u64_per_cols_2 = (level_2+63) / 64; // En entrée de la fonction

    const uint64_t total_colors = level_1 + level_2;
    uint64_t sparse_colors_bits = bits_needed(total_colors - 1);    
    if (sparse_colors_bits <= 8) sparse_colors_bits = 8;
    else if (sparse_colors_bits <= 16) sparse_colors_bits = 16;
    else if (sparse_colors_bits <= 32) sparse_colors_bits = 32;
    else sparse_colors_bits = 64;

    //max number of colors before it becomes dense -> before it uses as many bits as bitmap
    const uint64_t granularity = 64 / sparse_colors_bits;
    const uint64_t dense_threshold = 
        (n_u64_per_cols_1 + n_u64_per_cols_2 - 1) * granularity - 1; //-1 because we encode listsize aswell

    const uint64_t _iBuffA_ = (1 + n_u64_per_cols_1                  ) * 1024;
    const uint64_t _iBuffB_ = (1 +                   n_u64_per_cols_2) * 1024;
    const uint64_t _oBuff_  = (1 + n_u64_per_cols_1 + n_u64_per_cols_2) * 1024;
    const uint64_t _o_sparse_Buff_  = 10240;

    std::cerr << "n_u64_per_cols_1: " << n_u64_per_cols_1 << "\n";
    std::cerr << "n_u64_per_cols_2: " << n_u64_per_cols_2 << "\n";
    std::cerr << "sparse_colors_bits: " << sparse_colors_bits << "\n";
    std::cerr << "granularity: " << granularity << "\n";
    std::cerr << "dense_threshold: " << dense_threshold << "\n";
    std::cerr << "_iBuffA_: " << _iBuffA_ << "\n";
    std::cerr << "_iBuffB_: " << _iBuffB_ << "\n";
    std::cerr << "_oBuff_: " << _oBuff_ << "\n";
    std::cerr << "_o_sparse_Buff_: " << _o_sparse_Buff_ << "\n";

    uint64_t *in_1 = new uint64_t[_iBuffA_];
    uint64_t *in_2 = new uint64_t[_iBuffB_];
    uint64_t *dest = new uint64_t[_oBuff_ ];
    uint64_t *dest_sparse = new uint64_t[_oBuff_ ];

    uint64_t nElementsA = 0; // nombre d'éléments chargés en mémoire
    uint64_t nElementsB = 0; // nombre d'éléments chargés en mémoire

    uint64_t counterA = 0; // nombre de données lues dans le flux
    uint64_t counterB = 0; // nombre de données lues dans le flux
    uint64_t ndst     = 0; // nombre de données écrites dans le flux
    uint64_t ndst_sparse = 0; // nombre de données écrites dans le flux

    stream_reader* fin_1 = stream_reader_library::allocate( ifile_1 );
    stream_reader* fin_2 = stream_reader_library::allocate( ifile_2 );
    stream_writer* fdst  = stream_writer_library::allocate( o_file  );
    stream_writer* fdst_sparse  = stream_writer_library::allocate( o_file_sparse  );

    while (true) {
        if (counterA == nElementsA) {
            nElementsA = fin_1->read(in_1, sizeof(uint64_t), _iBuffA_);
            if (nElementsA == 0) {
                break;
            }
            counterA = 0;

        }
        if (counterB == nElementsB) {
            nElementsB = fin_2->read(in_2, sizeof(uint64_t), _iBuffB_);
            if (nElementsB == 0) {
                break;
            };
            counterB = 0;
        }

        while ((counterA != nElementsA) && (counterB != nElementsB) && ndst < (_oBuff_)) {
            const uint64_t v1        = in_1[counterA];
            const uint64_t v2        = in_2[counterB];

            ///////////
            // v1 = v2
            ///////////
            if (v1 == v2) {
                uint64_t density = 0; 
                for(int c = 0; c < n_u64_per_cols_1; c +=1){
                    density += popcount64( in_1[counterA + 1 + c] );
                }
                for(int c = 0; c < n_u64_per_cols_2; c +=1){
                    density += popcount64( in_2[counterB + 1 + c] );
                }

                if (density <= dense_threshold){ //encode colors w/ list of int
                    if (ndst_sparse + ((density+granularity-1) / granularity) >= _o_sparse_Buff_) {
                        fdst_sparse->write(dest_sparse, sizeof(uint64_t), ndst_sparse);
                        ndst_sparse = 0;
                    }

                    
                    dest_sparse[ndst_sparse++] = v1;
                    dest_sparse[ndst_sparse++] = density << (64 - sparse_colors_bits); //values are sent to MSB first
                    // encode colors from file 1
                    uint64_t cnt = 1;
                    for (int c = 0; c < n_u64_per_cols_1; ++c) {
                        uint64_t col_data = in_1[counterA + 1 + c];
                        while (col_data) {
                            unsigned int b = __builtin_ctzll(col_data);
                            uint64_t color_value = c * 64 + b;

                            uint64_t offset = (granularity - (cnt & (granularity - 1)) - 1) * sparse_colors_bits;

                            if ((cnt & (granularity - 1)) == 0) {
                                // = if (cnt % granularity == 0)
                                dest_sparse[ndst_sparse++] = (color_value << offset);
                            } else {
                                dest_sparse[ndst_sparse - 1] |= (color_value << offset);
                            }
                            cnt++;
                            // clear that bit
                            col_data &= col_data - 1;
                        }
                    }            

                    // encode colors from file 2
                    for (int c = 0; c < n_u64_per_cols_2; ++c) {
                        uint64_t col_data = in_2[counterB + 1 + c];
                        while (col_data) {
                            unsigned int b = __builtin_ctzll(col_data);
                            uint64_t color_value = c * 64 + b;

                            uint64_t offset = (granularity - (cnt & (granularity - 1)) - 1) * sparse_colors_bits;

                            if ((cnt & (granularity - 1)) == 0) {
                                // = if (cnt % granularity == 0)
                                dest_sparse[ndst_sparse++] = (color_value << offset);
                            } else {
                                dest_sparse[ndst_sparse - 1] |= (color_value << offset);
                            }
                            cnt++;
                            // clear that bit
                            col_data &= col_data - 1;
                        }
                    }            
                } 
                else { //encode w/ bitmap
                    dest[ndst++] = v1;
                    for(int c = 0; c < n_u64_per_cols_1; c +=1)
                        dest[ndst++] = in_1[counterA + 1 + c];
                    for(int c = 0; c < n_u64_per_cols_2; c +=1)
                        dest[ndst++] = in_2[counterB + 1 + c];
                }

                counterA  += (1 + n_u64_per_cols_1);
                counterB  += (1 + n_u64_per_cols_2);
            }

            ///////////
            // v1 < v2
            ///////////
            else if (v1 < v2) {
                uint64_t density = 0; 
                for(int c = 0; c < n_u64_per_cols_1; c +=1){
                    density += popcount64( in_1[counterA + 1 + c] );
                }

                if (density <= dense_threshold){ //encode colors w/ list of int
                    if (ndst_sparse + ((density+granularity-1) / granularity) >= _o_sparse_Buff_) {
                        fdst_sparse->write(dest_sparse, sizeof(uint64_t), ndst_sparse);
                        ndst_sparse = 0;
                    }

                    dest_sparse[ndst_sparse++] = v1;
                    dest_sparse[ndst_sparse++] = density << (64 - sparse_colors_bits);
                    // encode colors from file 1
                    uint64_t cnt = 1;

                    for (int c = 0; c < n_u64_per_cols_1; ++c) {
                        uint64_t col_data = in_1[counterA + 1 + c];
                        while (col_data) {
                            unsigned int b = __builtin_ctzll(col_data);
                            uint64_t color_value = c * 64 + b;

                            uint64_t offset = (granularity - (cnt & (granularity - 1)) - 1) * sparse_colors_bits;

                            if ((cnt & (granularity - 1)) == 0) {
                                // = if (cnt % granularity == 0)
                                dest_sparse[ndst_sparse++] = (color_value << offset);
                            } else {
                                dest_sparse[ndst_sparse - 1] |= (color_value << offset);
                            }
                            cnt++;
                            // clear that bit
                            col_data &= col_data - 1;
                        }
                    }                            
                } 
                else { //encode w/ bitmap
                    dest[ndst++] = v1;
                    for(int c = 0; c < n_u64_per_cols_1; c +=1)
                        dest[ndst++] = in_1[counterA + 1 + c];
                    for(int c = 0; c < n_u64_per_cols_2; c +=1)
                        dest[ndst++] = 0;
                }
                counterA  += (1 + n_u64_per_cols_1);
            } 
            
            ///////////
            // v1 > v2
            ///////////
            else { 
                uint64_t density = 0; 
                for(int c = 0; c < n_u64_per_cols_2; c +=1){
                    density += popcount64( in_2[counterB + 1 + c] );
                }

                if (density <= dense_threshold){ //encode colors w/ list of int
                    if (ndst_sparse + ((density+granularity-1) / granularity) >= _o_sparse_Buff_) {
                        fdst_sparse->write(dest_sparse, sizeof(uint64_t), ndst_sparse);
                        ndst_sparse = 0;
                    }

                    dest_sparse[ndst_sparse++] = v2;
                    dest_sparse[ndst_sparse++] = density << (64 - sparse_colors_bits);
                    // encode colors from file 1
                    uint64_t cnt = 1;

                    for (int c = 0; c < n_u64_per_cols_2; ++c) {
                        uint64_t col_data = in_2[counterB + 1 + c];
                        while (col_data) {
                            unsigned int b = __builtin_ctzll(col_data);
                            uint64_t color_value = c * 64 + b;

                            uint64_t offset = (granularity - (cnt & (granularity - 1)) - 1) * sparse_colors_bits;

                            if ((cnt & (granularity - 1)) == 0) {
                                // = if (cnt % granularity == 0)
                                dest_sparse[ndst_sparse++] = (color_value << offset);
                            } else {
                                dest_sparse[ndst_sparse - 1] |= (color_value << offset);
                            }
                            cnt++;
                            // clear that bit
                            col_data &= col_data - 1;
                        }
                    }                      
                } 
                else { //encode w/ bitmap
                    dest[ndst++] = v2;
                    for(int c = 0; c < n_u64_per_cols_2; c +=1)
                        dest[ndst++] = in_2[counterB + 1 + c];
                    for(int c = 0; c < n_u64_per_cols_2; c +=1)
                        dest[ndst++] = 0;
                }
                counterB  += (1 + n_u64_per_cols_2);
            }           
        }

        //
        // On flush le buffer en écriture
        //
        if (ndst >= _oBuff_) { // ATTENTION (>=) car on n'est pas tjs multiple
            fdst->write(dest, sizeof(uint64_t), ndst);
            ndst = 0;
        }

        
    }

    // remaining elements from file 2
    if (nElementsA == 0) {
        while (true) {
            if (counterB == nElementsB) {
                nElementsB = fin_2->read(in_2, sizeof(uint64_t), _iBuffB_);
                counterB = 0;
                if (nElementsB == 0) {
                    break;
                }
            }


            const uint64_t v2 = in_2[counterB];
            uint64_t density = 0; 
            for(int c = 0; c < n_u64_per_cols_2; c +=1){
                density += popcount64( in_2[counterB + 1 + c] );
            }

            if (density <= dense_threshold){ //encode colors w/ list of int
                if (ndst_sparse + ((density+granularity-1) / granularity) >= _o_sparse_Buff_) {
                    fdst_sparse->write(dest_sparse, sizeof(uint64_t), ndst_sparse);
                    ndst_sparse = 0;
                }

                dest_sparse[ndst_sparse++] = v2;
                dest_sparse[ndst_sparse++] = density << (64 - sparse_colors_bits);
                // encode colors from file 2
                uint64_t cnt = 1;

                for (int c = 0; c < n_u64_per_cols_2; ++c) {
                    uint64_t col_data = in_2[counterB + 1 + c];
                    while (col_data) {
                        unsigned int b = __builtin_ctzll(col_data);
                        uint64_t color_value = c * 64 + b;

                        uint64_t offset = (granularity - (cnt & (granularity - 1)) - 1) * sparse_colors_bits;

                        if ((cnt & (granularity - 1)) == 0) {
                            // = if (cnt % granularity == 0)
                            dest_sparse[ndst_sparse++] = (color_value << offset);
                        } else {
                            dest_sparse[ndst_sparse - 1] |= (color_value << offset);
                        }
                        cnt++;
                        // clear that bit
                        col_data &= col_data - 1;
                    }
                }            
            } 
            else { //encode w/ bitmap
                dest[ndst++] = v2;
                for(int c = 0; c < n_u64_per_cols_1; c +=1)
                    dest[ndst++] = 0;
                for(int c = 0; c < n_u64_per_cols_2; c +=1)
                    dest[ndst++] = in_2[counterB + 1 + c];

                if (ndst >= _oBuff_) { 
                    fdst->write(dest, sizeof(uint64_t), ndst);
                    ndst = 0;
                }
            }
           
            counterB  += (1 + n_u64_per_cols_2);
        }
    }

    // remaining elements from file 1
    else if (nElementsB == 0) {
        while (true) {
            if (counterA == nElementsA) {
                nElementsA = fin_1->read(in_1, sizeof(uint64_t), _iBuffA_);
                counterA = 0;
                if (nElementsA == 0) {
                    break;
                }
            }


            const uint64_t v1 = in_1[counterA];
            uint64_t density = 0; 
            for(int c = 0; c < n_u64_per_cols_1; c +=1){
                density += popcount64( in_1[counterA + 1 + c] );
            }

            if (density <= dense_threshold){ //encode colors w/ list of int
                if (ndst_sparse + ((density+granularity-1) / granularity) >= _o_sparse_Buff_) {
                    fdst_sparse->write(dest_sparse, sizeof(uint64_t), ndst_sparse);
                    ndst_sparse = 0;
                }

                dest_sparse[ndst_sparse++] = v1;
                dest_sparse[ndst_sparse++] = density << (64 - sparse_colors_bits);
                // encode colors from file 1
                uint64_t cnt = 1;

                for (int c = 0; c < n_u64_per_cols_1; ++c) {
                    uint64_t col_data = in_1[counterA + 1 + c];
                    while (col_data) {
                        unsigned int b = __builtin_ctzll(col_data);
                        uint64_t color_value = c * 64 + b;

                        uint64_t offset = (granularity - (cnt & (granularity - 1)) - 1) * sparse_colors_bits;

                        if ((cnt & (granularity - 1)) == 0) {
                            // = if (cnt % granularity == 0)
                            dest_sparse[ndst_sparse++] = (color_value << offset);
                        } else {
                            dest_sparse[ndst_sparse - 1] |= (color_value << offset);
                        }
                        cnt++;
                        // clear that bit
                        col_data &= col_data - 1;
                    }
                }                
            } 
            else { //encode w/ bitmap
                dest[ndst++] = v1;
                for(int c = 0; c < n_u64_per_cols_1; c +=1)
                    dest[ndst++] = in_1[counterA + 1 + c];
                for(int c = 0; c < n_u64_per_cols_2; c +=1)
                    dest[ndst++] = 0;

                if (ndst >= _oBuff_) { 
                    fdst->write(dest, sizeof(uint64_t), ndst);
                    ndst = 0;
                }
            }
           
            counterA  += (1 + n_u64_per_cols_1);
        }
    }

    fdst->write(dest, sizeof(uint64_t), ndst);
    fdst_sparse->write(dest_sparse, sizeof(uint64_t), ndst_sparse);

    delete fin_1;
    delete fin_2;
    delete fdst;
    delete fdst_sparse;

    delete [] in_1;
    delete [] in_2;
    delete [] dest;
    delete [] dest_sparse;
}
