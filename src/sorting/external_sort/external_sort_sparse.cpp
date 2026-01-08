#include "external_sort.hpp"
#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"
#include "../../../include/config.hpp"

#include <algorithm>
#include <vector>
#include <queue>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <omp.h>
#include <atomic>
#include <memory>
#include <stdexcept> // Added for std::runtime_error

// =========================================================================
// Data Structures
// =========================================================================

struct Element {
    uint64_t minimizer;
    std::vector<uint64_t> payload; // [Header (contains size), ...Body...]
};

// Comparator for sorting elements by payload (color sets)
struct ElementCmp {
    bool operator()(const Element& a, const Element& b) const {
        return a.payload < b.payload;
    }
};

// =========================================================================
// Buffered Page Reader
// =========================================================================

class BufferedPageReader {
public:
    BufferedPageReader(const std::string& filename, uint64_t bits_per_color, size_t buffer_size_bytes = 1024 * 1024 * 8)
        : _bits_per_color(bits_per_color),
          _colors_per_word(64 / bits_per_color),
          _pos(0),
          _limit(0),
          _eof_reached(false) 
    {
        _reader.reset(stream_reader_library::allocate(filename));
        if (!_reader || !_reader->is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }

        // --- FIX 1: Cap Buffer to 1GB to prevent Integer Overflow in read() ---
        // The underlying reader uses 'int' (max ~2GB). We cap at 1GB to be safe.
        size_t max_safe_bytes = 1024ULL * 1024ULL * 1024ULL;
        if (buffer_size_bytes > max_safe_bytes) {
            buffer_size_bytes = max_safe_bytes;
        }

        // Allocate buffer (ensure multiple of uint64_t)
        size_t num_words = std::max((size_t)1024, buffer_size_bytes / sizeof(uint64_t));
        _buffer.resize(num_words);

        // Initial fill
        refill();
    }

    ~BufferedPageReader() {
        if (_reader) _reader->close();
    }

    BufferedPageReader(const BufferedPageReader&) = delete;
    BufferedPageReader& operator=(const BufferedPageReader&) = delete;

    bool next(Element& out) {
        // 1. We need at least 2 words to start (Minimizer + Header)
        if (!ensure_available(2)) {
            return false;
        }

        // 2. Decode size from Header (2nd word in buffer relative to pos)
        uint64_t header = _buffer[_pos + 1];
        uint64_t list_size = header >> (64 - _bits_per_color);
        size_t n_payload_words = (list_size + _colors_per_word) / _colors_per_word;
        size_t total_words_needed = 1 + n_payload_words;

        // 3. Ensure the FULL element is in the buffer
        if (!ensure_available(total_words_needed)) {
             // If this fails, the element is larger than the entire buffer.
             // This can happen if buffer_size is too small in merge phase.
             throw std::runtime_error("Sparse element size exceeds buffer size! Increase min buffer.");
        }

        // 4. Extract Data
        out.minimizer = _buffer[_pos++];
        out.payload.resize(n_payload_words);
        std::memcpy(out.payload.data(), &_buffer[_pos], n_payload_words * sizeof(uint64_t));
        _pos += n_payload_words;

        return true;
    }

    size_t fill_batch(std::vector<Element>& batch, size_t max_words) {
        size_t words_read = 0;
        Element el;
        while (words_read < max_words) {
            if (!next(el)) break;
            words_read += (1 + el.payload.size());
            batch.push_back(std::move(el));
        }
        return words_read;
    }

private:
    std::unique_ptr<stream_reader> _reader;
    const uint64_t _bits_per_color;
    const uint64_t _colors_per_word;
    
    std::vector<uint64_t> _buffer;
    size_t _pos;    
    size_t _limit;  
    bool _eof_reached;

    bool ensure_available(size_t n) {
        if (_pos + n <= _limit) return true;
        if (_eof_reached) return false;

        // Move remaining data to start
        size_t remaining = _limit - _pos;
        if (remaining > 0) {
            std::memmove(_buffer.data(), &_buffer[_pos], remaining * sizeof(uint64_t));
        }
        
        _pos = 0;
        _limit = remaining;

        // Read new data
        size_t words_capacity = _buffer.size() - _limit;
        
        // This cast is now safe because we capped _buffer size in constructor
        int words_read_count = _reader->read(
            (void*)(_buffer.data() + _limit), 
            sizeof(uint64_t), 
            (int)words_capacity
        );

        if (words_read_count > 0) {
            _limit += words_read_count;
        }

        if (words_read_count < (int)words_capacity || _reader->is_eof()) {
            _eof_reached = true;
        }

        return _limit >= n;
    }

    void refill() {
        _pos = 0;
        _limit = 0;
        _eof_reached = false;
        ensure_available(1);
    }
};

// =========================================================================
// Verification Logic
// =========================================================================

bool check_file_sorted_sparse(const std::string& filename, uint64_t bits_per_color, bool verbose_flag) {
    uint64_t idx = 0;
    try {
        BufferedPageReader reader(filename, bits_per_color);
        Element prev, curr;
        
        if (!reader.next(prev)) {
            if (verbose_flag) std::cout << "File is empty.\n";
            return true;
        }

        while (reader.next(curr)) {
            if (prev.payload > curr.payload) {
                std::cout << "File not sorted at index " << idx + 1 << "\n";
                return false;
            }
            prev = std::move(curr);
            idx++;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error checking file: " << e.what() << "\n";
        return false;
    }

    if (verbose_flag) std::cout << "File " << filename << " is correctly sorted (" << idx << " elements).\n";
    return true;
}

// =========================================================================
// Phase 1: Create Sorted Chunks
// =========================================================================

void write_chunk(const std::vector<Element>& batch, const std::string& filename) {
    std::unique_ptr<stream_writer> writer(stream_writer_library::allocate(filename));
    if (!writer || !writer->is_open()) throw std::runtime_error("Cannot open output chunk: " + filename);

    for (const auto& el : batch) {
        writer->write((void*)&el.minimizer, sizeof(uint64_t), 1);
        writer->write((void*)el.payload.data(), sizeof(uint64_t), el.payload.size());
    }
    writer->close();
}

std::vector<std::string> parallel_create_chunks(
    const std::string& infile,
    const std::string& tmp_dir,
    uint64_t max_words_in_RAM,
    int bits_per_color,
    int verbose,
    int num_threads
) {
    std::vector<std::string> chunk_files;
    BufferedPageReader reader(infile, bits_per_color); // 1GB max buffer implicitly
    omp_lock_t read_lock;
    omp_init_lock(&read_lock);

    std::atomic<int> chunk_counter{0};
    bool done = false;
    uint64_t ram_per_thread = std::max((uint64_t)2000, max_words_in_RAM / num_threads);

    #pragma omp parallel num_threads(num_threads) shared(done, chunk_files)
    {
        while (true) {
            std::vector<Element> batch;
            batch.reserve(ram_per_thread / 4); 

            omp_set_lock(&read_lock);
            if (done) {
                omp_unset_lock(&read_lock);
                break;
            }
            // reader is NOT thread safe, but protected by lock here.
            reader.fill_batch(batch, ram_per_thread);
            if (batch.empty()) {
                done = true;
                omp_unset_lock(&read_lock);
                break;
            }
            omp_unset_lock(&read_lock);

            std::sort(batch.begin(), batch.end(), ElementCmp());

            int id = chunk_counter.fetch_add(1);
            std::string outname = tmp_dir + "/sparse_chunk_" + std::to_string(id) + ".lz4";
            write_chunk(batch, outname);

            #pragma omp critical
            chunk_files.push_back(outname);
        }
    }
    omp_destroy_lock(&read_lock);
    return chunk_files;
}

// =========================================================================
// Phase 2: N-Way Merge
// =========================================================================

struct MergeNode {
    Element el;
    int stream_idx;
    bool operator>(const MergeNode& other) const { return el.payload > other.el.payload; }
};

void nway_merge_sparse(
    const std::vector<std::string>& chunk_files,
    const std::string& outfile,
    int bits_per_color,
    int verbose,
    uint64_t ram_budget_bytes
) {
    int n_files = chunk_files.size();
    if (n_files == 0) return;

    if (verbose >= 3) std::cout << "[III] Merging " << n_files << " chunks.\n";

    size_t buffer_size_per_file = (ram_budget_bytes / (n_files + 1)); 
    
    // --- FIX 2: Ensure Minimum Buffer Size ---
    // Increase min size to ~64KB. This ensures that even with massive fan-in,
    // we don't accidentally drop "large" sparse elements.
    if (buffer_size_per_file < 65536) {
        buffer_size_per_file = 65536; 
        if (verbose >= 3) std::cout << "[Warning] RAM tight, enforcing 64KB min buffer per file.\n";
    }

    // The Constructor of BufferedPageReader will cap this at 1GB automatically (Fix 1)
    std::vector<std::unique_ptr<BufferedPageReader>> readers;
    readers.reserve(n_files);

    for (const auto& f : chunk_files) {
        readers.push_back(std::make_unique<BufferedPageReader>(f, bits_per_color, buffer_size_per_file));
    }

    std::priority_queue<MergeNode, std::vector<MergeNode>, std::greater<MergeNode>> pq;

    for (int i = 0; i < n_files; ++i) {
        Element el;
        if (readers[i]->next(el)) {
            pq.push({std::move(el), i});
        }
    }

    std::unique_ptr<stream_writer> writer(stream_writer_library::allocate(outfile));
    if (!writer || !writer->is_open()) throw std::runtime_error("Cannot open output: " + outfile);

    while (!pq.empty()) {
        MergeNode node = pq.top();
        pq.pop();

        writer->write((void*)&node.el.minimizer, sizeof(uint64_t), 1);
        writer->write((void*)node.el.payload.data(), sizeof(uint64_t), node.el.payload.size());

        if (readers[node.stream_idx]->next(node.el)) {
            pq.push(std::move(node));
        }
    }
    writer->close();
}

void external_sort_sparse(const std::string& infile, const std::string& outfile, const std::string& tmp_dir,
                          const uint64_t n_colors, const uint64_t ram_value_MB, const bool keep_tmp_files,
                          const int verbose, int n_threads) {
    uint64_t bits_per_color = std::ceil(std::log2(n_colors));
    if (bits_per_color <= 8) bits_per_color = 8;
    else if (bits_per_color <= 16) bits_per_color = 16;
    else if (bits_per_color <= 32) bits_per_color = 32;
    else bits_per_color = 64;

    const uint64_t bytes_in_RAM = ram_value_MB * 1024ULL * 1024ULL;
    const uint64_t max_words_in_RAM = bytes_in_RAM / sizeof(uint64_t);

    auto chunk_files = parallel_create_chunks(infile, tmp_dir, max_words_in_RAM, bits_per_color, verbose, n_threads);

    if (chunk_files.empty()) {
        std::unique_ptr<stream_writer> w(stream_writer_library::allocate(outfile)); w->close();
    } else if (chunk_files.size() == 1) {
        std::filesystem::rename(chunk_files[0], outfile);
    } else {
        nway_merge_sparse(chunk_files, outfile, bits_per_color, verbose, bytes_in_RAM);
    }

    if (!keep_tmp_files) {
        for (const auto& f : chunk_files) std::filesystem::remove(f);
    }
}