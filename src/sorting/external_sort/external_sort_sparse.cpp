#include "external_sort.hpp"

#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"
#include "../../../include/config.hpp"


struct Element {
    uint64_t minimizer;
    std::vector<uint64_t> payload; // includes list_size + colors
};


bool element_color_cmp(const Element& a, const Element& b) {
    return a.payload < b.payload;
}

struct FileElement {
    Element el;
    FILE* file;
    size_t file_idx;
};

// comparison for min-heap based on element_color_cmp
struct CompareElement {
    bool operator()(const FileElement& a, const FileElement& b) const {
        return element_color_cmp(b.el, a.el); // min-heap
    }
};




static int fread_element(FILE* f, Element &out, uint64_t bits_per_color, uint64_t colors_per_word) 
{
    uint64_t minimizer;
    if (fread(&minimizer, sizeof(uint64_t), 1, f) != 1) return 0; // EOF

    uint64_t second_word;
    fread(&second_word, sizeof(uint64_t), 1, f);

    // extract list_size from MSB bits
    uint64_t list_size = second_word >> (64 - bits_per_color);
    size_t n_color_words = (list_size + colors_per_word) / colors_per_word; //think about encoding list size aswwell

    // payload = [second_word, possibly more words]
    out.minimizer = minimizer;
    out.payload.resize(n_color_words);
    out.payload[0] = second_word;

    if (n_color_words > 1){
        fread(out.payload.data() + 1, sizeof(uint64_t), n_color_words - 1, f);
    }
    return (1+n_color_words);
}


bool check_file_sorted_sparse(
    const std::string& filename,
    uint64_t bits_per_color,
    bool verbose_flag
) {
    FILE* f = fopen(filename.c_str(), "rb");
    if (!f) {
        std::cout << "Error: cannot open " << filename << " for reading.\n";
        return false;
    }

    uint64_t colors_per_word = 64 / bits_per_color;

    Element el;
    fread_element(f, el, bits_per_color, colors_per_word); // read first element

    uint64_t idx = 0;

    while (true) {
        Element e2;
        int got = fread_element(f, e2, bits_per_color, colors_per_word);
        if (got == 0) break;

        // compare el and e2
        if (element_color_cmp(e2, el)) {
            std::cout << "File not sorted by color at element index " << idx + 1
                      << " (0-based). Previous color > current color.\n";
            if (verbose_flag) {
                std::cout << "Prev color: ";
                for (size_t j = 0; j < el.payload.size(); ++j) std::cout << el.payload[j] << " ";
                std::cout << "\nCurr color: ";
                for (size_t j = 0; j < e2.payload.size(); ++j) std::cout << e2.payload[j] << " ";
                std::cout << std::endl;
            }
            fclose(f);
            return false;
        }
        el = std::move(e2);
        idx++;
    }

    fclose(f);
    if (verbose_flag) {
        std::cout << "File " << filename << " is correctly sorted by color ("
                  << idx << " elements checked).\n";
    }
    return true;
}


std::vector<Element> read_elements_batch(FILE* fin,
                                         uint64_t max_words_in_RAM,
                                         int bits_per_color)
{
    std::vector<Element> elements;

    const int colors_per_word = 64 / bits_per_color;
    uint64_t total_words = 0;

    while (total_words < max_words_in_RAM) {
        Element el;
        int n_words = fread_element(fin, el, bits_per_color, colors_per_word);
        if (n_words == 0) // EOF
            break;

        elements.push_back(std::move(el));
        total_words += n_words;
    }

    return elements;
}

void write_sorted_chunk(const std::vector<Element>& batch, const std::string& filename) {
    FILE* fout = fopen(filename.c_str(), "wb");
    if (!fout)
        throw std::runtime_error("Cannot open output file: " + filename);

    for (const auto& el : batch) {
        fwrite(&el.minimizer, sizeof(uint64_t), 1, fout);
        fwrite(el.payload.data(), sizeof(uint64_t), el.payload.size(), fout);
    }

    fclose(fout);
}

std::vector<std::string> parallel_create_sparse_chunks(const std::string& infile,
                                   const std::string& tmp_dir,
                                   uint64_t max_words_in_RAM,
                                   int bits_per_color,
                                   int verbose,
                                   int num_threads)       
{
    std::vector<std::string> chunk_files;
    FILE* fin = fopen(infile.c_str(), "rb");
    if (!fin)
        throw std::runtime_error("Cannot open input file " + infile);

    omp_lock_t read_lock;
    omp_init_lock(&read_lock);

    std::atomic<size_t> chunk_idx{0};
    bool done = false; //security for multithreading

    // use multiple chunks (and thus threads) only if file is bigger than RAM
    if (get_file_size(infile) / sizeof(uint64_t) > max_words_in_RAM){
        max_words_in_RAM = max_words_in_RAM / num_threads;
    }
    
    if (verbose >= 3){
        std::cout << "[III] max_words_in_RAM per thread: " << max_words_in_RAM << "\n";
    }

    #pragma omp parallel num_threads(num_threads) shared(done, chunk_idx)
    {
        while (true) {
            std::vector<Element> batch;

            omp_set_lock(&read_lock);
            if (!done) {
                batch = read_elements_batch(fin,  max_words_in_RAM, bits_per_color);
                if (batch.empty())
                    done = true;
            }
            omp_unset_lock(&read_lock);

            if (done || batch.empty())
                break;

            size_t idx = chunk_idx.fetch_add(1);

            std::sort(batch.begin(), batch.end(), element_color_cmp);

            std::string outname = tmp_dir + "/sparse_chunk_" + std::to_string(idx) + ".bin";
            write_sorted_chunk(batch, outname);

            chunk_files.push_back(outname);
        }
    }

    omp_destroy_lock(&read_lock);
    fclose(fin);

    if (verbose >= 3) {
        std::cout << "[III] All chunks written (" << chunk_idx << " total)\n";
    }

    return chunk_files;
}


void nway_merge_sparse(const std::vector<std::string>& chunk_files,
                       const std::string& outfile,
                       int bits_per_color,
                       int verbose,
                       uint64_t ram_budget_bytes = 512ULL * 1024ULL * 1024ULL) // optional RAM budget
{
    const size_t n_files = chunk_files.size();
    if (n_files == 0) return;

    const uint64_t colors_per_word = 64 / bits_per_color;
    const int shift_amount = 64 - bits_per_color;

    // 1. Memory Management
    const size_t total_buf_words = ram_budget_bytes / sizeof(uint64_t);
    const size_t buf_words_per_file = total_buf_words / (n_files + 1); 

    if (verbose >= 3) {
        std::cout << "[III] [nway_merge_sparse] RAM: " << ram_budget_bytes/(1024*1024)
                  << " MB, per-input buffer: " << buf_words_per_file
                  << " words, output buffer: " << buf_words_per_file << " words\n";
    }

    // 2. Open Input Handles
    std::vector<FILE*> handles(n_files, nullptr);
    for (size_t i = 0; i < n_files; ++i) {
        handles[i] = fopen(chunk_files[i].c_str(), "rb");
        if (!handles[i]) {
            // Cleanup already opened
            for (size_t k = 0; k < i; ++k) if(handles[k]) fclose(handles[k]);
            throw std::runtime_error("Cannot open chunk file: " + chunk_files[i]);
        }
    }

    // 3. Open Output Handle
    FILE* fout = fopen(outfile.c_str(), "wb");
    if (!fout) {
        for (auto f : handles) if(f) fclose(f);
        throw std::runtime_error("Cannot open output file: " + outfile);
    }

    // 4. Initialize Buffers
    std::vector<std::vector<uint64_t>> in_bufs(n_files, std::vector<uint64_t>(buf_words_per_file));
    std::vector<size_t> buf_pos(n_files, 0);      // Current cursor in buffer
    std::vector<size_t> buf_limit(n_files, 0);    // Valid data limit in buffer
    std::vector<bool> eof_reached(n_files, false);

    // --- Helper: Refill Buffer ---
    auto refill = [&](size_t i) -> bool {
        if (eof_reached[i]) return false;
        
        size_t n_read = fread(in_bufs[i].data(), sizeof(uint64_t), in_bufs[i].size(), handles[i]);
        buf_pos[i] = 0;
        buf_limit[i] = n_read;
        
        if (n_read == 0) {
            eof_reached[i] = true;
            return false;
        }
        return true;
    };

    // --- Helper: Read One Element Safe ---
    auto read_element_buffered = [&](size_t i, Element& out) -> bool {
        if (buf_pos[i] >= buf_limit[i]) {
            if (!refill(i)) return false; // Real EOF, natural end of file
        }
        out.minimizer = in_bufs[i][buf_pos[i]++];

        if (buf_pos[i] >= buf_limit[i]) {
            if (!refill(i)) return false; // Error: File ended between minimizer and payload
        }

        uint64_t second_word = in_bufs[i][buf_pos[i]++];
        uint64_t list_size = second_word >> shift_amount;
        size_t n_color_words = (list_size + colors_per_word) / colors_per_word;

        out.payload.resize(n_color_words);
        out.payload[0] = second_word;

        if (n_color_words > 1) {
            size_t words_needed = n_color_words - 1;
            size_t words_collected = 0;

            while (words_collected < words_needed) {
                // Refill if buffer empty
                if (buf_pos[i] >= buf_limit[i]) {
                    if (!refill(i)) return false; // Error: File ended in middle of payload
                }

                size_t available = buf_limit[i] - buf_pos[i];
                size_t to_copy = std::min(available, words_needed - words_collected);

                std::copy(in_bufs[i].begin() + buf_pos[i],
                          in_bufs[i].begin() + buf_pos[i] + to_copy,
                          out.payload.begin() + 1 + words_collected);

                buf_pos[i] += to_copy;
                words_collected += to_copy;
            }
        }

        return true;
    };

    // 5. Prime the Priority Queue
    std::priority_queue<FileElement, std::vector<FileElement>, CompareElement> pq;
    for (size_t i = 0; i < n_files; ++i) {
        refill(i); // Initial fill
        Element el;
        if (read_element_buffered(i, el)) {
            pq.push({std::move(el), handles[i], i});
        }
    }

    // 6. Output Buffer Setup
    std::vector<uint64_t> out_buf(buf_words_per_file);
    size_t out_pos = 0;

    auto flush_output = [&]() {
        if (out_pos > 0) {
            fwrite(out_buf.data(), sizeof(uint64_t), out_pos, fout);
            out_pos = 0;
        }
    };

    size_t merge_count = 0;

    // 7. Main Loop
    while (!pq.empty()) {
        FileElement fe = pq.top();
        pq.pop();

        // Calculate space needed: 1 word (minimizer) + N words (payload)
        size_t needed = 1 + fe.el.payload.size();

        if (out_pos + needed > out_buf.size()) {
            flush_output();
        }

        // Write Minimizer
        out_buf[out_pos++] = fe.el.minimizer;
        
        // Write Payload (using fast copy)
        std::copy(fe.el.payload.begin(), fe.el.payload.end(), out_buf.begin() + out_pos);
        out_pos += fe.el.payload.size();

        merge_count++;

        // Get next element from the same file
        Element next;
        if (read_element_buffered(fe.file_idx, next)) {
            pq.push({std::move(next), fe.file, fe.file_idx});
        }
    }

    // 8. Cleanup
    flush_output();
    fclose(fout);
    for (auto f : handles) if(f) fclose(f);

    if (verbose >= 3) {
        std::cout << "[III] Merged " << merge_count << " elements into " << outfile << "\n";
    }
}


// Full external sort orchestrator
void external_sort_sparse(const std::string& infile,
                          const std::string& outfile,
                          const std::string& tmp_dir,
                          const uint64_t n_colors,
                          const uint64_t ram_value_MB,
                          const bool keep_tmp_files,
                          const int verbose,
                          int n_threads)
{
    uint64_t bits_per_color = std::ceil(std::log2(n_colors));
    if (bits_per_color <= 8) bits_per_color = 8;
    else if (bits_per_color <= 16) bits_per_color = 16;
    else if (bits_per_color <= 32) bits_per_color = 32;
    else bits_per_color = 64;

    const uint64_t bytes_in_RAM = ram_value_MB * 1024ULL * 1024ULL;
    const uint64_t max_words_in_RAM = bytes_in_RAM / sizeof(uint64_t);

    if (verbose >= 3) {
        std::cout << "[III] sparse External sort parameters:\n";
        std::cout << "\t\t bits_per_color: " << bits_per_color << "\n";
        std::cout << "\t\t n_colors: " << n_colors << "\n";
        std::cout << "\t\t RAM limit: " << ram_value_MB << " MB (" << max_words_in_RAM << " words)\n";
        std::cout << "\t\t threads: " << n_threads << "\n";
    }

    // Phase 1: Create sorted chunks in parallel
    auto chunk_files = parallel_create_sparse_chunks(infile, tmp_dir, max_words_in_RAM, bits_per_color, verbose, n_threads);


    if (verbose >= 3) {
        std::cout << "[III] Merging " << chunk_files.size() << " chunks...\n";

    }
        
    if (chunk_files.size() == 1) {
        // Single chunk, just rename
        std::filesystem::rename(chunk_files[0], outfile);
        if (verbose >= 3) {
            std::cout << "[III] Only one chunk created, renamed to " << outfile << "\n";
        }
            
        return;
    }

    // Phase 2: Merge
    nway_merge_sparse(chunk_files, outfile, bits_per_color, verbose, bytes_in_RAM);

    // Cleanup temporary files if requested
    if (!keep_tmp_files) {
        for (const auto& f : chunk_files) std::filesystem::remove(f);
        if (verbose >= 3) std::cout << "[III] Temporary files removed.\n";
    }

    if (verbose >= 3)
        std::cout << "[III] External sort complete → " << outfile << "\n";
}
