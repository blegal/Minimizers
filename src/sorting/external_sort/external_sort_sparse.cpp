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
        std::cerr << "Error: cannot open " << filename << " for reading.\n";
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
            std::cerr << "File not sorted by color at element index " << idx + 1
                      << " (0-based). Previous color > current color.\n";
            if (verbose_flag) {
                std::cerr << "Prev color: ";
                for (size_t j = 0; j < el.payload.size(); ++j) std::cerr << el.payload[j] << " ";
                std::cerr << "\nCurr color: ";
                for (size_t j = 0; j < e2.payload.size(); ++j) std::cerr << e2.payload[j] << " ";
                std::cerr << std::endl;
            }
            fclose(f);
            return false;
        }
        el = std::move(e2);
        idx++;
    }

    fclose(f);
    if (verbose_flag) {
        std::cerr << "File " << filename << " is correctly sorted by color ("
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
                                   bool verbose_flag,
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

    max_words_in_RAM = max_words_in_RAM / num_threads;

    std::cerr << "max_words_in_RAM per thread: " << max_words_in_RAM << "\n";

    #pragma omp parallel num_threads(num_threads) shared(done, chunk_idx)
    {
        int tid = omp_get_thread_num();

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

            if (verbose_flag && tid == 0)
                std::cout << "[Thread " << tid << "] Sorting chunk #" << idx
                          << " (" << batch.size() << " elements)\n";

            std::sort(batch.begin(), batch.end(), element_color_cmp);

            std::string outname = tmp_dir + "/sparse_chunk_" + std::to_string(idx) + ".bin";
            write_sorted_chunk(batch, outname);

            chunk_files.push_back(outname);

            if (verbose_flag)
                std::cout << "[Thread " << tid << "] Wrote " << outname << "\n";
        }
    }

    omp_destroy_lock(&read_lock);
    fclose(fin);

    if (verbose_flag)
        std::cout << "All chunks written (" << chunk_idx << " total)\n";

    return chunk_files;
}

// Merge multiple sorted chunk files into one output file
void nway_merge_sparse(const std::vector<std::string>& chunk_files,
                       const std::string& outfile,
                       int bits_per_color,
                       bool verbose_flag)
{
    const uint64_t colors_per_word = 64 / bits_per_color;

    std::priority_queue<FileElement, std::vector<FileElement>, CompareElement> pq;
    std::vector<FILE*> handles(chunk_files.size(), nullptr);

    // Open all chunk files and push first element of each into heap
    for (size_t i = 0; i < chunk_files.size(); ++i) {
        FILE* f = fopen(chunk_files[i].c_str(), "rb");
        if (!f) throw std::runtime_error("Cannot open chunk file: " + chunk_files[i]);
        handles[i] = f;

        Element el;
        if (fread_element(f, el, bits_per_color, colors_per_word) > 0) {
            pq.push({std::move(el), f, i});
        }
    }

    FILE* fout = fopen(outfile.c_str(), "wb");
    if (!fout) throw std::runtime_error("Cannot open output file: " + outfile);

    size_t merge_count = 0;
    while (!pq.empty()) {
        FileElement top = std::move(const_cast<FileElement&>(pq.top()));
        pq.pop();

        // write smallest element
        fwrite(&top.el.minimizer, sizeof(uint64_t), 1, fout);
        fwrite(top.el.payload.data(), sizeof(uint64_t), top.el.payload.size(), fout);
        merge_count++;

        // read next element from same file
        Element next;
        if (fread_element(top.file, next, bits_per_color, colors_per_word) > 0) {
            pq.push({std::move(next), top.file, top.file_idx});
        }
    }

    fclose(fout);
    for (FILE* f : handles)
        if (f) fclose(f);

    if (verbose_flag)
        std::cout << "Merged " << merge_count << " elements into " << outfile << "\n";
}

// Full external sort orchestrator
void external_sort_sparse(const std::string& infile,
                          const std::string& outfile,
                          const std::string& tmp_dir,
                          const uint64_t n_colors,
                          const uint64_t ram_value_MB,
                          const bool keep_tmp_files,
                          const bool verbose_flag,
                          int n_threads)
{
    uint64_t bits_per_color = std::ceil(std::log2(n_colors));
    if (bits_per_color <= 8) bits_per_color = 8;
    else if (bits_per_color <= 16) bits_per_color = 16;
    else if (bits_per_color <= 32) bits_per_color = 32;
    else bits_per_color = 64;

    const uint64_t bytes_in_RAM = ram_value_MB * 1024ULL * 1024ULL;
    const uint64_t max_words_in_RAM = bytes_in_RAM / sizeof(uint64_t);

    if (verbose_flag) {
        std::cout << "External sort parameters:\n";
        std::cout << " - bits_per_color: " << bits_per_color << "\n";
        std::cout << " - n_colors: " << n_colors << "\n";
        std::cout << " - RAM limit: " << ram_value_MB << " MB (" << max_words_in_RAM << " words)\n";
        std::cout << " - threads: " << n_threads << "\n";
    }

    // Phase 1: Create sorted chunks in parallel
    auto chunk_files = parallel_create_sparse_chunks(infile, tmp_dir, max_words_in_RAM, bits_per_color, verbose_flag, n_threads);


    if (verbose_flag)
        std::cout << "Merging " << chunk_files.size() << " chunks...\n";

    // Phase 2: Merge
    nway_merge_sparse(chunk_files, outfile, bits_per_color, verbose_flag);

    // Cleanup temporary files if requested
    if (!keep_tmp_files) {
        for (const auto& f : chunk_files) std::filesystem::remove(f);
        if (verbose_flag) std::cout << "Temporary files removed.\n";
    }

    if (verbose_flag)
        std::cout << "External sort complete → " << outfile << "\n";
}
