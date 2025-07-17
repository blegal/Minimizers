#include "external_sort.hpp"

#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"
#include "../../../include/config.hpp"

#include <random>
#include <chrono>
#include <array>


struct element { 
    uint64_t minmer;    
    std::array<uint64_t, N_UINT_PER_COLOR> colors;
};



uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}

bool compare_elements(const element& e1, const element& e2) {
    for (size_t i = 0; i < N_UINT_PER_COLOR; ++i) {
        if (e1.colors[i] != e2.colors[i]) {
            return e1.colors[i] < e2.colors[i];
        }
    }
    return e1.minmer > e2.minmer;
}


void print_first_n_elements(const std::string& filename, size_t n) {
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
    for (size_t i = 0; i < n; ++i) {
        element e;
        size_t read1 = fread(&e.minmer, sizeof(uint64_t), 1, f);
        size_t read2 = fread(e.colors.data(), sizeof(uint64_t), N_UINT_PER_COLOR, f);
        if (read1 != 1 || read2 != N_UINT_PER_COLOR) {
            std::cout << "End of file reached after " << i << " elements." << std::endl;
            break;
        }
        std::cout << "Element " << i << ": minmer = " << e.minmer << ", colors = [";
        for (size_t j = 0; j < N_UINT_PER_COLOR; ++j) {
            std::cout << e.colors[j];
            if (j + 1 < N_UINT_PER_COLOR) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
    fclose(f);
}

void random_example(
    const std::string& outfile,
    size_t n_elements
) {
    std::vector<element> random_elements;
    random_elements.reserve(n_elements);
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> minmer_dist(0, 1000000000);
    std::uniform_int_distribution<uint64_t> color_dist(0, 20);

    for (size_t i = 0; i < n_elements; ++i) {
        element e;
        e.minmer = minmer_dist(gen);
        for (size_t j = 0; j < N_UINT_PER_COLOR; ++j) {
            e.colors[j] = color_dist(gen);
        }
        random_elements.push_back(e);
    }

    FILE* fout = fopen(outfile.c_str(), "wb");
    if (!fout) {
        std::cerr << "Error opening file for writing elements." << std::endl;
        return;
    }
    for (const auto& e : random_elements) {
        fwrite(&e.minmer, sizeof(uint64_t), 1, fout);
        fwrite(e.colors.data(), sizeof(uint64_t), N_UINT_PER_COLOR, fout);
    }
    fclose(fout);
}


void internal_sort(
    const std::string& infile,
    const std::string& outfile
) {
    // for comparison

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    const uint64_t size_bytes = get_file_size(infile);
    const uint64_t n_elements = size_bytes / sizeof(uint64_t) / (N_UINT_PER_COLOR + 1);

    
    FILE* fi = fopen(infile.c_str(), "rb");
    if (!fi) {
        std::cerr << "Error opening file: " << infile << std::endl;
        return;
    }
    std::vector<element> elements;
    elements.resize(n_elements);

    size_t read_count = fread(elements.data(), sizeof(element), n_elements, fi);
    if (read_count != n_elements) {
        std::cerr << "Error reading file: " << infile << std::endl;
        fclose(fi);
        return;
    }
    fclose(fi);

    std::sort(elements.begin(), elements.end(), compare_elements);

    FILE* fo = fopen(outfile.c_str(), "wb");
    if (!fo) {
        std::cerr << "Error opening file for writing: " << outfile << std::endl;
        return;
    }

    for (const auto& elem : elements) {
        fwrite(&elem.minmer, sizeof(uint64_t), 1, fo);
        fwrite(elem.colors.data(), sizeof(uint64_t), N_UINT_PER_COLOR, fo);
    }
    fclose(fo);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Internal sorting took: " << elapsed.count() << " seconds." << std::endl;

}    



void external_sort(
    const std::string& infile,
    const std::string& outfile,
    const std::string& tmp_dir,
    const uint64_t n_colors,
    const uint64_t ram_value,
    const bool keep_tmp_files,
    const bool verbose_flag)
{
    // Implementation of external sort algorithm
    // This function will handle the sorting of large files
    // using external memory techniques.
    
    if (verbose_flag) {
        std::cout << "Starting external sort on file: " << infile << std::endl;
    }    

    //
    // INPUT DATA
    //
    const uint64_t size_bytes = get_file_size(infile);

    const uint64_t n_uint_per_element = N_UINT_PER_COLOR + 1;

    const uint64_t n_elements = size_bytes / sizeof(uint64_t) / n_uint_per_element;

    const uint64_t bytes_per_element = n_uint_per_element * sizeof(uint64_t);
    uint64_t max_elements_in_RAM = (ram_value * 1024 * 1024) / bytes_per_element;

    uint64_t n_chunks = n_elements / max_elements_in_RAM;
    if (n_elements % max_elements_in_RAM != 0) {
        n_chunks += 1;
    }

    if (verbose_flag) {
        std::cout << "File size in bytes: " << size_bytes << std::endl;
        std::cout << "# uint64_t elements: " << n_elements << std::endl;
        std::cout << "# minimizers: " << n_elements / n_uint_per_element << std::endl;
        std::cout << "# of colors: " << n_colors << std::endl;
        std::cout << "# uint64_t/colors: " << n_uint_per_element - 1 << std::endl;
        std::cout << "Max elements per chunk/in RAM: " << max_elements_in_RAM << std::endl;
        std::cout << "# chunks: " << n_chunks << std::endl;
    }

    //
    // CREATE TMP CHUNKS
    //
    std::vector<std::string> chunknames(n_chunks);
    if (true)
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        FILE* fi = fopen(infile.c_str(), "rb");
        if (!fi) {
            std::cerr << "Error opening file: " << infile << std::endl;
            return;
        }

        for (size_t chunk=0; chunk < n_chunks; chunk++){
            std::vector<element> minimizer_color(max_elements_in_RAM);

            size_t n_read = fread(
                minimizer_color.data(), 
                sizeof(element), 
                max_elements_in_RAM, 
                fi);
            std::cerr << "Read " << n_read << " elements for chunk " << chunk << std::endl;
            minimizer_color.resize(n_read);

            std::sort(minimizer_color.begin(), minimizer_color.end(), compare_elements);

            std::string tmp_filename = tmp_dir + "/chunk_" + std::to_string(chunk) + ".bin";
            chunknames[chunk] = tmp_filename;

            FILE* fo = fopen(tmp_filename.c_str(), "wb");
            if (!fo) {
                std::cerr << "Error creating temporary file: " << tmp_filename << std::endl
                            << " during external sort." << std::endl;
                fclose(fi);
                return;
            }
            for (const auto& elem : minimizer_color) {
                fwrite(&elem.minmer, sizeof(uint64_t), 1, fo);
                fwrite(&elem.colors, sizeof(uint64_t), N_UINT_PER_COLOR, fo);
            }
            fclose(fo);
        }

        fclose(fi);

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        if (verbose_flag) {
            std::cout << "Chunk creation took: " << elapsed.count() << " seconds." << std::endl;
        } 
    } //end of chunk creation


    //
    // N-WAY MERGE SORTED CHUNKS
    //
    if (true) {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        const uint64_t n_elements_per_buff = max_elements_in_RAM / (n_chunks+1); // need to open n_chunks input buffer + 1 output buffer


        // Open chunks
        std::vector<stream_reader*> i_files (n_chunks);
        for(size_t i = 0; i < n_chunks; i += 1)
        {
            stream_reader* f = stream_reader_library::allocate( chunknames[i] );
            if( f == NULL )
            {
                printf("(EE) File does not exist (%s))\n", chunknames[i].c_str());
                printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
                exit( EXIT_FAILURE );
            }
            i_files[i] = f;
        }

        // Create buffers for reading
        std::vector< std::vector<element> > i_buffer(i_files.size());
        for(size_t i = 0; i < i_files.size(); i += 1)
            i_buffer[i] = std::vector<element>(n_elements_per_buff);

        // Create output buffer
        std::vector<element> dest(n_elements_per_buff);

        // Create vector to store number of elements in each buffer
        std::vector<int64_t> nElements(i_files.size());
        for(size_t i = 0; i < i_files.size(); i += 1)
            nElements[i] = 0;

        int64_t ndst = 0; // Number of elements in output buffer

        // Create vector to store current position in each buffer
        std::vector<int64_t> counter(i_files.size());
        for(size_t i = 0; i < i_files.size(); i += 1)
            counter[i] = 0;

        // Open output file
        stream_writer* fdst = stream_writer_library::allocate( outfile );
        if( fdst == NULL )
        {
            printf("(EE) File does not exist (%s))\n", outfile.c_str());
            printf("(EE) Error location : %s %d\n", __FILE__, __LINE__);
            exit( EXIT_FAILURE );
        }

        
        while (true) {
            // check data for every buff
            for(size_t i = 0; i < i_files.size(); i += 1)
            {
                // try to reload
                if (counter[i] == nElements[i])
                {
                    nElements[i] = i_files[i]->read(i_buffer[i].data(), sizeof(element), n_elements_per_buff);

                    std::cerr << "Read " << nElements[i] << " elements for chunk " << i << std::endl;

                    counter  [i] = 0;
                    if( nElements[i] == 0 )
                    {
                        // eof, delete the file
                        delete i_files[i];
                        i_files.erase  ( i_files.begin()   + i );
                        i_buffer.erase ( i_buffer.begin()  + i );
                        nElements.erase( nElements.begin() + i );
                        counter.erase  ( counter.begin()   + i );
                        i -= 1;
                    }
                }
            }

            // no more flux = no more merge
            if(i_files.size() == 0)
                break;


            uint64_t curr_index;
            
            do{
                // look for min in all buffers
                curr_index = -1; // = max value
                element* curr_ptr = nullptr;

                for(size_t i = 0; i < i_files.size(); i += 1)
                {
                    const int pos  = counter[i];
                    element* buf  = &i_buffer[i][pos];
                    if (curr_ptr == nullptr || compare_elements(*buf, *curr_ptr))
                    {
                        curr_ptr = buf;
                        curr_index = i;
                    }
                }


                dest[ndst++] = *curr_ptr;
                counter[curr_index] += 1;
                
                if (ndst == n_elements_per_buff) {
                    //fwrite(dest, sizeof(uint64_t), ndst - 1 - oSize, fdst);
                    fdst->write(dest.data(), sizeof(element), ndst);
                    ndst = 0;
                }

            }while( counter[curr_index] != nElements[curr_index] );

        }

        if (ndst != 0) {
            fdst->write(dest.data(), sizeof(element), ndst);
            ndst = 0;
        }

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        if (verbose_flag) {
            std::cout << "N-way merge took: " << elapsed.count() << " seconds." << std::endl;
        }

        delete fdst;

        if (!keep_tmp_files) {
            for (const auto& fname : chunknames) {
                std::remove(fname.c_str());
                std::remove(infile.c_str());
            }
        }
    } //end of n-way merge
}

int main(){
    std::string infile = "/home/vlevallo/tmp/test_bertrand/random_1M_elements.bin";
    std::string outfile = "/home/vlevallo/tmp/test_bertrand/sorted_elements.bin";
    std::string tmp_dir = "/home/vlevallo/tmp/test_bertrand/tmp";

    std::uint64_t nb_elements = 1000000; // Example number of elements
    std::cout << "Number of elements: " << nb_elements << std::endl;
    
    random_example(infile, nb_elements);

    internal_sort(infile, outfile);

    external_sort(infile, outfile, tmp_dir, 32*64, 100, true, true);

    print_first_n_elements(outfile, 10);
}