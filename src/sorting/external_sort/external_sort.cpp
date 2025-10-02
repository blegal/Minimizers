#include "external_sort.hpp"

#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"
#include "../../../include/config.hpp"

#include <random>
#include <chrono>
#include <array>
#include <cstring>


struct element {
    uint64_t* data; // points to the start of the element

    // Compare colors lexicographically (skip minimizer)
    bool operator<(const element& other) const {
        return std::memcmp(data + 1, other.data + 1, (size_t)data[0] * sizeof(uint64_t)) < 0;
    }
};



uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}


/* void print_first_n_elements(const std::string& filename, size_t n) {
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
} */

/* void random_example(
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
} */


/* void internal_sort(
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

    FILE* fo = fopen(outfile.c_str(), "wb+");
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

}     */

struct ChunkEntry {
    std::vector<uint64_t> buffer;
    size_t pos = 0;
    size_t count = 0;
    FILE* f = nullptr;
};

// comparator for heap
struct EntryCmp {
    const uint64_t n_uint_per_element;
    EntryCmp(uint64_t n) : n_uint_per_element(n) {}
    bool operator()(const std::pair<uint64_t*, size_t>& a,
                    const std::pair<uint64_t*, size_t>& b) const {
        return memcmp(a.first + 1, b.first + 1,
                      (n_uint_per_element - 1) * sizeof(uint64_t)) > 0;
    }
};

void nway_merge(const std::vector<std::string>& chunknames,
                const std::string& outfile,
                uint64_t n_uint_per_element,
                uint64_t max_elements_in_RAM,
                bool verbose_flag) {

    // Buffers per chunk
    size_t n_chunks = chunknames.size();
    uint64_t buf_elems = max_elements_in_RAM / (n_chunks + 1); // fair share

    std::vector<ChunkEntry> chunks(n_chunks);
    for (size_t i = 0; i < n_chunks; i++) {
        chunks[i].buffer.resize(buf_elems * n_uint_per_element);
        chunks[i].f = fopen(chunknames[i].c_str(), "rb");
        if (!chunks[i].f) throw std::runtime_error("Cannot open chunk file");
        chunks[i].count = fread(chunks[i].buffer.data(),
                              n_uint_per_element * sizeof(uint64_t),
                              buf_elems,
                              chunks[i].f);
        chunks[i].pos = 0;
    }

    FILE* fout = fopen(outfile.c_str(), "wb");
    if (!fout) throw std::runtime_error("Cannot open output file");
    std::vector<uint64_t> outbuf(buf_elems * n_uint_per_element);

    // Min-heap over (pointer to element, chunk_index)
    std::vector<std::pair<uint64_t*, size_t>> heap;
    for (size_t i = 0; i < n_chunks; i++) {
        if (chunks[i].count > 0) {
            heap.push_back({ chunks[i].buffer.data(), i });
        }
    }
    EntryCmp cmp(n_uint_per_element);
    std::make_heap(heap.begin(), heap.end(), cmp);

    size_t outcount = 0;

    while (!heap.empty()) {
        std::pop_heap(heap.begin(), heap.end(), cmp);
        auto [ptr, idx] = heap.back();
        heap.pop_back();

        // write element to output buffer
        memcpy(&outbuf[outcount * n_uint_per_element],
               ptr,
               n_uint_per_element * sizeof(uint64_t));
        outcount++;

        if (outcount == buf_elems) {
            fwrite(outbuf.data(),
                   n_uint_per_element * sizeof(uint64_t),
                   buf_elems,
                   fout);
            outcount = 0;
        }

        // advance this chunk
        chunks[idx].pos++;
        if (chunks[idx].pos == chunks[idx].count) {
            // refill buffer
            chunks[idx].count = fread(chunks[idx].buffer.data(),
                                    n_uint_per_element * sizeof(uint64_t),
                                    buf_elems,
                                    chunks[idx].f);
            chunks[idx].pos = 0;
            if (chunks[idx].count == 0) {
                fclose(chunks[idx].f);
                continue;
            }
        }
        heap.push_back({ chunks[idx].buffer.data() + chunks[idx].pos * n_uint_per_element, idx });
        std::push_heap(heap.begin(), heap.end(), cmp);
    }

    if (outcount > 0) {
        fwrite(outbuf.data(),
               n_uint_per_element * sizeof(uint64_t),
               outcount,
               fout);
    }

    fclose(fout);

    if (verbose_flag) {
        std::cerr << "N-way merge complete into: " << outfile << std::endl;
    }
}

//to try : create element<>() for element seen & only half mem used to sort, other one to reorder and writ one single chunk of memory

void external_sort(
    const std::string& infile,
    const std::string& outfile,
    const std::string& tmp_dir,
    const uint64_t n_colors,
    const uint64_t ram_value_MB,
    const bool keep_tmp_files,
    const bool verbose_flag)
{
    // Implementation of external sort algorithm
    // This function will handle the sorting of large files
    // using external memory techniques.
    // outfile contains header(1 uint64_t) with number of distinct color-vectors, then sorted elements
    if (verbose_flag) {
        std::cerr << "Starting external sort on file: " << infile << std::endl;
    }    

    //
    // INPUT DATA
    //
    const uint64_t size_bytes           = get_file_size(infile);
    const uint64_t n_uint_per_element   = (n_colors + 63) / 64 + 1;
    const uint64_t n_elements           = size_bytes / sizeof(uint64_t) / n_uint_per_element;
    const uint64_t bytes_per_element    = n_uint_per_element * sizeof(uint64_t);
    uint64_t max_elements_in_RAM        = static_cast<uint64_t>((ram_value_MB * 1024 * 1024 * 0.9) / bytes_per_element);
    //only 90% of RAM for buffer, then 10% for pointers to swap

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
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        FILE* fi = fopen(infile.c_str(), "rb");
        if (!fi) {
            std::cerr << "Error opening file: " << infile << std::endl;
            return;
        }

        auto cmp = [&](const uint64_t* a, const uint64_t* b) {
            return memcmp(a + 1, b + 1, (n_uint_per_element - 1) * sizeof(uint64_t)) < 0;
        };

        for (size_t chunk=0; chunk < n_chunks; chunk++){
            std::vector<uint64_t> input_buffer(max_elements_in_RAM * n_uint_per_element);

            size_t got = fread(
                input_buffer.data(), 
                bytes_per_element, 
                max_elements_in_RAM, 
                fi);

            std::cerr << "Read " << got << " elements for chunk " << chunk << std::endl;
            //input_buffer.resize(got);
            std::vector<uint64_t*> ptrs(got);
            for (size_t i = 0; i < got; i++) {
                ptrs[i] = &input_buffer[i * n_uint_per_element];
            }
                
            sort(ptrs.begin(), ptrs.end(), cmp);

            std::string tmp_filename = tmp_dir + "/chunk_" + std::to_string(chunk) + ".bin";
            chunknames[chunk] = tmp_filename;

            FILE* fout = fopen(tmp_filename.c_str(), "wb");
            if (!fout) {
                std::cerr << "Error creating temporary file: " << tmp_filename << std::endl
                            << " during external sort." << std::endl;
                fclose(fi);
                return;
            }
            for (size_t i = 0; i < got; i++) {
                fwrite(ptrs[i], bytes_per_element, 1, fout);
            }
            fclose(fout);
        }

        fclose(fi);

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        if (verbose_flag) {
            std::cout << "Chunks creation took: " << elapsed.count() << " seconds." << std::endl;
        } 
    } //end of chunk creation


    //
    // N-WAY MERGE SORTED CHUNKS
    //
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        nway_merge(chunknames, outfile, n_uint_per_element, max_elements_in_RAM, verbose_flag);

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        if (verbose_flag) {
            std::cout << "nway merge took: " << elapsed.count() << " seconds." << std::endl;
        } 

        if (!keep_tmp_files) {
            for (const auto& fname : chunknames) {
                std::remove(fname.c_str());
            }
            std::remove(infile.c_str());
        }
    } //end of n-way merge
}

/* int main(){
    std::string infile = "/home/vlevallo/tmp/test_bertrand/data_n0.3682c";
    std::string outfile = "/home/vlevallo/tmp/test_bertrand/sorted_elements.bin";
    std::string tmp_dir = "/home/vlevallo/tmp/test_bertrand/tmp";
    
    //random_example(infile, nb_elements);

    //internal_sort(infile, outfile);

    external_sort(infile, outfile, tmp_dir, 3682, 1024, true, true);

    //print_first_n_elements(outfile, 10);

    return 0;
}  */