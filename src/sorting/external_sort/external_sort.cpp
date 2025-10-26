#include "external_sort.hpp"

#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"
#include "../../../include/config.hpp"



// ------------------------------------------------------------
// Utility
// ------------------------------------------------------------
void generate_random_example(const std::string& filename,
                             double size_in_MB,
                             uint64_t num_colors,
                             bool verbose = true) {
    // Compute how many uint64_t we need per element
    uint64_t n_uint_color = (num_colors + 63) / 64; // ceil division
    uint64_t n_uint_per_element = 1 + n_uint_color; // 1 for minimizer

    // Total size in bytes
    uint64_t target_bytes = static_cast<uint64_t>(size_in_MB * 1024.0 * 1024.0);
    uint64_t elem_size = n_uint_per_element * sizeof(uint64_t);
    uint64_t num_elements = target_bytes / elem_size;

    if (num_elements == 0)
        throw std::runtime_error("Requested size too small to contain one element");

    if (verbose) {
        std::cout << "Generating file: " << filename << "\n";
        std::cout << "  Target size: " << size_in_MB << " MB\n";
        std::cout << "  Colors: " << num_colors << " (" << n_uint_color << " uint64s per color set)\n";
        std::cout << "  Elements: " << num_elements << "\n";
        std::cout << "  Each element = " << n_uint_per_element << " uint64_t (" << elem_size << " bytes)\n";
    }

    std::ofstream fout(filename, std::ios::binary);
    if (!fout)
        throw std::runtime_error("Cannot open file for writing: " + filename);

    std::mt19937_64 rng(42); // fixed seed for reproducibility
    std::uniform_int_distribution<uint64_t> dist_minimizer(0, UINT64_MAX);
    std::uniform_int_distribution<uint64_t> dist_bits(0, UINT64_MAX);

    std::vector<uint64_t> buffer(n_uint_per_element * 4096); // write in blocks
    size_t written = 0;

    while (written < num_elements) {
        size_t block_elems = std::min<size_t>(4096, num_elements - written);
        for (size_t i = 0; i < block_elems; i++) {
            size_t base = i * n_uint_per_element;
            buffer[base] = dist_minimizer(rng); // random minimizer
            for (uint64_t j = 1; j < n_uint_per_element; j++) {
                buffer[base + j] = dist_bits(rng); // random color bits
            }
        }
        fout.write(reinterpret_cast<char*>(buffer.data()),
                   block_elems * elem_size);
        written += block_elems;
    }

    fout.close();

    if (verbose) {
        double actual_MB = (double)(num_elements * elem_size) / (1024.0 * 1024.0);
        std::cout << "✅ Done. Wrote ~" << actual_MB << " MB (" << num_elements << " elements).\n";
    }
}

bool check_file_sorted(
    const std::string& filename,
    uint64_t n_uint_per_element,
    bool verbose_flag = true
) {
    if (n_uint_per_element < 2) {
        std::cerr << "Error: n_uint_per_element must be >= 2 (minimizer + at least one color word).\n";
        return false;
    }

    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) {
        std::cerr << "Error: cannot open " << filename << " for reading.\n";
        return false;
    }

    const size_t bytes_per_element = n_uint_per_element * sizeof(uint64_t);
    const size_t color_bytes = (n_uint_per_element - 1) * sizeof(uint64_t);

    // Choose buffer size ~1MB but aligned to an integer number of elements
    const size_t target_bytes = 1 << 20; // 1 MiB
    size_t elems_per_buf = std::max<size_t>(1, target_bytes / bytes_per_element);
    // ensure buffer fits at least one element
    std::vector<uint64_t> buffer(elems_per_buf * n_uint_per_element);

    std::vector<uint64_t> prev_color(n_uint_per_element - 1);
    bool have_prev = false;
    uint64_t element_index = 0;

    while (true) {
        size_t got = fread(buffer.data(), bytes_per_element, elems_per_buf, f);
        if (got == 0) break;

        for (size_t i = 0; i < got; ++i) {
            uint64_t* elem_ptr = buffer.data() + i * n_uint_per_element;
            uint64_t* color_ptr = elem_ptr + 1; // skip minimizer

            if (have_prev) {
                // Compare bytewise (consistent with memcmp used elsewhere)
                int cmp = memcmp(prev_color.data(),
                                 color_ptr,
                                 color_bytes);
                if (cmp > 0) { // prev_color > current_color  => not sorted
                    std::cerr << "File not sorted by color at element index " << element_index
                              << " (0-based). Previous color > current color.\n";
                    if (verbose_flag) {
                        std::cerr << "Prev color: ";
                        for (size_t j = 0; j < prev_color.size(); ++j) std::cerr << prev_color[j] << " ";
                        std::cerr << "\nCurr color: ";
                        for (size_t j = 0; j < (n_uint_per_element - 1); ++j) std::cerr << color_ptr[j] << " ";
                        std::cerr << std::endl;
                    }
                    fclose(f);
                    return false;
                }
            } else {
                have_prev = true;
            }

            // copy current color into prev_color for next iteration
            memcpy(prev_color.data(), color_ptr, color_bytes);
            ++element_index;
        }
    }

    fclose(f);
    if (verbose_flag) {
        std::cerr << "File " << filename << " is correctly sorted by color ("
                  << element_index << " elements checked).\n";
    }
    return true;
}

uint64_t get_file_size(const std::string& filen) {
    struct stat file_status;
    if (stat(filen.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}

// ------------------------------------------------------------
// ChunkEntry + comparator structs
// ------------------------------------------------------------
struct ChunkEntry {
    std::vector<uint64_t> buffer;
    size_t pos = 0;
    size_t count = 0;
    FILE* f = nullptr;
};

struct EntryCmp {
    const uint64_t n_uint_per_element;
    EntryCmp(uint64_t n) : n_uint_per_element(n) {}
    bool operator()(const std::pair<uint64_t*, size_t>& a,
                    const std::pair<uint64_t*, size_t>& b) const {
        return memcmp(a.first + 1, b.first + 1,
                      (n_uint_per_element - 1) * sizeof(uint64_t)) > 0;
    }
};


std::vector<std::string> parallel_create_chunks(const std::string& infile,
                                                const std::string& tmp_dir,
                                                uint64_t n_uint_per_element,
                                                uint64_t max_elements_in_RAM,
                                                uint64_t n_chunks,
                                                uint64_t bytes_per_element,
                                                bool verbose_flag,
                                                int n_threads)
{
    std::vector<std::string> chunknames(n_chunks);
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    // Each thread will have its own local buffer and file access
    uint64_t elements_per_thread = max_elements_in_RAM / n_threads;

    if (verbose_flag) {
        std::cerr << "Launching parallel chunk creation with " << n_threads << " threads." << std::endl;
    }

#pragma omp parallel for num_threads(n_threads) schedule(dynamic)
    for (size_t chunk = 0; chunk < n_chunks; chunk++) {
        try {
            // Compute offset for this chunk in the input file
            uint64_t offset_bytes = chunk * elements_per_thread * bytes_per_element;

            // Each thread opens its own FILE* and seeks to its region
            FILE* fi = fopen(infile.c_str(), "rb");
            if (!fi) {
                throw std::runtime_error("Cannot open input file " + infile);
            }
            fseek(fi, offset_bytes, SEEK_SET);

            std::vector<uint64_t> input_buffer(elements_per_thread * n_uint_per_element);
            size_t got = fread(input_buffer.data(), bytes_per_element, elements_per_thread, fi);
            fclose(fi);

            if (got == 0) continue;

            auto cmp = [&](const uint64_t* a, const uint64_t* b) {
                return memcmp(a + 1, b + 1, (n_uint_per_element - 1) * sizeof(uint64_t)) < 0;
            };

            std::vector<uint64_t*> ptrs(got);
            for (size_t i = 0; i < got; i++) {
                ptrs[i] = &input_buffer[i * n_uint_per_element];
            }

            std::sort(ptrs.begin(), ptrs.end(), cmp);

            std::string tmp_filename = tmp_dir + "/chunk_" + std::to_string(chunk) + ".bin";
            chunknames[chunk] = tmp_filename;

            FILE* fout = fopen(tmp_filename.c_str(), "wb");
            if (!fout) {
                throw std::runtime_error("Error creating file " + tmp_filename);
            }
            for (size_t i = 0; i < got; i++) {
                fwrite(ptrs[i], bytes_per_element, 1, fout);
            }
            fclose(fout);

            /* if (verbose_flag) {
                #pragma omp critical
                {
                    std::cerr << "Thread " << omp_get_thread_num()
                              << " finished chunk " << chunk
                              << " (" << got << " elements)." << std::endl;
                }
            } */

        } catch (const std::exception& e) {
            #pragma omp critical
            {
                std::cerr << "Error in thread " << omp_get_thread_num()
                          << " for chunk " << chunk << ": " << e.what() << std::endl;
            }
        }
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    if (verbose_flag) {
        std::cout << "Parallel chunks creation took: " << elapsed.count() << " seconds." << std::endl;
    }

    return chunknames;
}

void nway_merge(const std::vector<std::string>& chunknames,
                const std::string& outfile,
                uint64_t n_uint_per_element,
                uint64_t max_elements_in_RAM,
                bool verbose_flag) 
{

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    size_t n_chunks = chunknames.size();

    // Allocate RAM evenly: one portion per chunk + one for output buffer
    uint64_t buf_elems = max_elements_in_RAM / (n_chunks + 1);

    if (buf_elems == 0)
        throw std::runtime_error("Not enough RAM to allocate buffers for all chunks and output.");

    // Prepare output buffer
    std::vector<uint64_t> outbuf(buf_elems * n_uint_per_element);

    // Open chunks and fill initial buffers
    std::vector<ChunkEntry> chunks(n_chunks);
    for (size_t i = 0; i < n_chunks; i++) {
        chunks[i].buffer.resize(buf_elems * n_uint_per_element);
        chunks[i].f = fopen(chunknames[i].c_str(), "rb");
        if (!chunks[i].f) throw std::runtime_error("Cannot open chunk file: " + chunknames[i]);
        chunks[i].count = fread(chunks[i].buffer.data(),
                                n_uint_per_element * sizeof(uint64_t),
                                buf_elems,
                                chunks[i].f);
        chunks[i].pos = 0;
    }

    // Open output file
    FILE* fout = fopen(outfile.c_str(), "wb");
    if (!fout) throw std::runtime_error("Cannot open output file: " + outfile);

    // Initialize heap
    std::vector<std::pair<uint64_t*, size_t>> heap;
    for (size_t i = 0; i < n_chunks; i++) {
        if (chunks[i].count > 0) heap.push_back({ chunks[i].buffer.data(), i });
    }
    EntryCmp cmp(n_uint_per_element);
    std::make_heap(heap.begin(), heap.end(), cmp);

    size_t outcount = 0;

    while (!heap.empty()) {
        std::pop_heap(heap.begin(), heap.end(), cmp);
        auto [ptr, idx] = heap.back();
        heap.pop_back();

        // Copy element to output buffer
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

        // Advance this chunk
        chunks[idx].pos++;
        if (chunks[idx].pos == chunks[idx].count) {
            // Refill buffer
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

    // Flush remaining elements
    if (outcount > 0) {
        fwrite(outbuf.data(),
               n_uint_per_element * sizeof(uint64_t),
               outcount,
               fout);
    }

    fclose(fout);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    if (verbose_flag) {
        std::cout << "N-way merge took: " << elapsed.count() << " seconds." << std::endl;
        std::cerr << "N-way merge complete into: " << outfile << std::endl;
    }
}

void multi_pass_nway_merge(
    std::vector<std::string> chunknames,   // copy because we overwrite it
    const std::string& outfile,
    uint64_t n_uint_per_element,
    uint64_t max_elements_in_RAM,
    uint64_t total_data_bytes,
    double access_time_s = 0.0001,   // SSD access time ~0.1 ms
    double transfer_rate_bytes_per_s = 500.0 * 1024 * 1024, // 500 MB/s
    bool verbose_flag = true) 
{
    if (chunknames.empty())
        throw std::runtime_error("multi_pass_nway_merge: empty chunk list");

    size_t pass_num = 0;

    while (chunknames.size() > 1) {
        pass_num++;
        size_t n_chunks = chunknames.size();


        // IN A FEW WORDS : Estimate optimal fan-in based on RAM and I/O model (SSD)
        // Total RAM available for all buffers
        uint64_t ram_bytes = max_elements_in_RAM * n_uint_per_element * sizeof(uint64_t);

        // Buffer size per input chunk
        double buffer_size = double(ram_bytes) / (n_chunks+1);

        // Estimate total access time for reads
        double num_buffer_reads = double(total_data_bytes) / buffer_size;
        double total_access_time = num_buffer_reads * access_time_s;

        // Total transfer time (reads + writes)
        double transfer_time = double(total_data_bytes) / transfer_rate_bytes_per_s;
        double io_time = total_access_time + transfer_time;

        // Decide fan-in based on fraction of access time
        size_t fan_in;
        if (total_access_time > 0.5 * io_time) { // access time significant
            // Use square root of number of chunks as fan-in (at least 1, at most n_chunks)
            fan_in = std::min<size_t>(n_chunks, std::sqrt(static_cast<double>(n_chunks)));
        } else {
            fan_in = n_chunks; // merge all chunks at once
        }


        size_t n_groups = (n_chunks + fan_in - 1) / fan_in;

        if (verbose_flag) {
            std::cerr << "Pass " << pass_num << ": " << n_chunks << " chunks, "
                      << "buffer per chunk = " << buffer_size << " bytes, "
                      << "estimated access fraction = " << (total_access_time/io_time)
                      << ", fan-in = " << fan_in << ", groups = " << n_groups << "\n";
        }

        std::vector<std::string> tmp_files(n_groups);
        for (size_t g = 0; g < n_groups; ++g) {
            size_t start_idx = g * fan_in;
            size_t end_idx = std::min(start_idx + fan_in, n_chunks);
            std::vector<std::string> group_chunks(chunknames.begin() + start_idx,
                                                  chunknames.begin() + end_idx);

            tmp_files[g] = outfile + "_tmp_pass" + std::to_string(pass_num) + "_group" + std::to_string(g) + ".bin";

            if (verbose_flag) {
                std::cerr << "  Merging group " << g << " (" << group_chunks.size() << " chunks) -> "
                          << tmp_files[g] << "\n";
            }

            // Single-pass merge for this group
            nway_merge(group_chunks, tmp_files[g], n_uint_per_element, max_elements_in_RAM, verbose_flag);
        }

        // Remove old chunk files
        if (pass_num > 1) {
            for (const auto& f : chunknames) std::remove(f.c_str());
        }

        // Prepare for next pass
        chunknames = std::move(tmp_files);
    }

    // Final merge (only one chunk remains after iterations, rename to outfile)
    if (verbose_flag) {
        std::cerr << "Final merge result -> " << outfile << "\n";
    }
    if (!chunknames.empty() && chunknames[0] != outfile) {
        std::rename(chunknames[0].c_str(), outfile.c_str());
    }
}


void external_sort( const std::string& infile,
                    const std::string& outfile,
                    const std::string& tmp_dir,
                    const uint64_t n_colors,
                    const uint64_t ram_value_MB,
                    const bool keep_tmp_files,
                    const bool verbose_flag,
                    int n_threads)
{
    if (verbose_flag) {
        std::cerr << "Starting parallel external sort on file: " << infile << std::endl;
    }

    const uint64_t size_bytes         = get_file_size(infile);
    const uint64_t n_uint_per_element = (n_colors + 63) / 64 + 1;
    const uint64_t n_elements         = size_bytes / sizeof(uint64_t) / n_uint_per_element;
    const uint64_t bytes_per_element  = n_uint_per_element * sizeof(uint64_t);
    uint64_t max_elements_in_RAM      = static_cast<uint64_t>((ram_value_MB * 1024 * 1024) / bytes_per_element);

    uint64_t n_chunks = n_elements / (max_elements_in_RAM / n_threads);
    if (n_elements % (max_elements_in_RAM / n_threads) != 0) {
        n_chunks += 1;
    }

    if (verbose_flag) {
        std::cout << "File size in bytes: " << size_bytes << std::endl;
        std::cout << "# uint64_t elements: " << n_elements << std::endl;
        std::cout << "Max elements per thread chunk: " << (max_elements_in_RAM / n_threads) << std::endl;
        std::cout << "# chunks: " << n_chunks << std::endl;
    }

    // Phase 1: parallel chunk creation
    auto chunknames = parallel_create_chunks(infile, tmp_dir,
                                             n_uint_per_element,
                                             max_elements_in_RAM,
                                             n_chunks,
                                             bytes_per_element,
                                             verbose_flag,
                                             n_threads);

    // Phase 2: sequential n-way merge (can be parallelized later)
    multi_pass_nway_merge(chunknames, outfile,
                         n_uint_per_element,
                         max_elements_in_RAM,
                         size_bytes,
                         0.0001,               // access time ~0.1 ms
                         500.0 * 1024 * 1024,  // transfer rate ~500 MB/s
                         verbose_flag);


    // Cleanup
    if (!keep_tmp_files) {
        for (const auto& fname : chunknames) {
            std::remove(fname.c_str());
        }
        std::remove(infile.c_str());
    }
}


#if 1
int main(){
    std::string infile = "/home/vlevallo/tmp/test_bertrand/data_n0.3682c";
    std::string random_file = "/home/vlevallo/tmp/test_bertrand/random.10000c";
    std::string outfile = "/home/vlevallo/tmp/test_bertrand/sorted_elements.bin";
    std::string para_outfile = "/home/vlevallo/tmp/test_bertrand/parallel_sorted_elements.bin";
    std::string tmp_dir = "/home/vlevallo/tmp/test_bertrand/tmp";
    std::size_t ram_value = 512; //in MB
    
    //random_example(infile, nb_elements);

    //internal_sort(infile, outfile);

    //external_sort(random_file, para_outfile, tmp_dir, 10000, ram_value, true, true, 8);
    //check_file_sorted(para_outfile, (10000 + 63) / 64 + 1, true);

    external_sort_sparse(
        tmp_dir + "/data_n_final_sparse.3682c",
        outfile,
        tmp_dir,
        3682,
        256,
        true,
        true,
        8
    );


    //print_first_n_elements(outfile, 10);

    return 0;
}   
#endif



/*14359968+14301344+14276240+14271824+14246368+14258064+14311408+14247232+14308160+14370096+13662320+14379536+14329296+14290592+14302592+14372336+14360432+14302576+14278240+14316992
*/