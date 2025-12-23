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

/**
 * Wraps stream_reader to read large chunks of memory and parse variable-length
 * elements efficiently. Handles buffer boundary truncation.
 */
class BufferedPageReader {
public:
    BufferedPageReader(const std::string& filename, uint64_t bits_per_color, size_t buffer_size_bytes = 1024 * 1024 * 8)
        : _bits_per_color(bits_per_color),
          _colors_per_word(64 / bits_per_color),
          _pos(0),
          _limit(0),
          _eof_reached(false) 
    {
        // Use the library factory to allocate the specific reader (lz4/raw/etc)
        _reader.reset(stream_reader_library::allocate(filename));
        if (!_reader || !_reader->is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
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

    // Disable copy
    BufferedPageReader(const BufferedPageReader&) = delete;
    BufferedPageReader& operator=(const BufferedPageReader&) = delete;

    /**
     * Tries to parse the next element from the internal buffer.
     * Returns false if EOF is reached.
     */
    bool next(Element& out) {
        // 1. We need at least 2 words to start (Minimizer + Header)
        if (!ensure_available(2)) {
            return false;
        }

        // 2. Decode size from Header (2nd word in buffer relative to pos)
        uint64_t header = _buffer[_pos + 1];
        
        // B MSB bits encode the list size
        uint64_t list_size = header >> (64 - _bits_per_color);
        
        // Calculate total payload words (Header included in this count)
        // Logic: (Size + Capacity_Per_Word) / Capacity_Per_Word
        // Example: If Capacity=4. Size=1 -> 1 word. Size=4 -> 1 word. Size=5 -> 2 words.
        size_t n_payload_words = (list_size + _colors_per_word) / _colors_per_word;
        
        // Total words for this element = Minimizer (1) + Payload (n)
        size_t total_words_needed = 1 + n_payload_words;

        // 3. Ensure the FULL element is in the buffer
        if (!ensure_available(total_words_needed)) {
             // If we can't get the full element, the file is likely truncated/corrupt
             return false;
        }

        // 4. Extract Data
        out.minimizer = _buffer[_pos++];

        out.payload.resize(n_payload_words);
        std::memcpy(out.payload.data(), &_buffer[_pos], n_payload_words * sizeof(uint64_t));
        _pos += n_payload_words;

        return true;
    }

    /**
     * Reads elements until the batch limit (RAM usage) is reached.
     * Used for creating chunks.
     */
    size_t fill_batch(std::vector<Element>& batch, size_t max_words) {
        size_t words_read = 0;
        Element el;
        
        // Reserve strictly necessary to avoid reallocations if possible
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
    size_t _pos;    // Current cursor index in _buffer
    size_t _limit;  // Number of valid words currently in _buffer
    bool _eof_reached;

    /**
     * Ensures `n` words are available starting at `_pos`.
     * If not, moves remaining data to start of buffer and reads more from file.
     */
    bool ensure_available(size_t n) {
        if (_pos + n <= _limit) {
            return true; // Data is already there
        }

        if (_eof_reached) {
            return false; // Cannot fetch more
        }

        // 1. Shift remaining data to the beginning
        size_t remaining = _limit - _pos;
        if (remaining > 0) {
            std::memmove(_buffer.data(), &_buffer[_pos], remaining * sizeof(uint64_t));
        }
        
        _pos = 0;
        _limit = remaining;

        // 2. Read new data to fill the rest of the buffer
        size_t words_capacity = _buffer.size() - _limit;
        
        // stream_reader::read returns number of elements read
        int words_read_count = _reader->read(
            (void*)(_buffer.data() + _limit), 
            sizeof(uint64_t), 
            (int)words_capacity
        );

        if (words_read_count > 0) {
            _limit += words_read_count;
        }

        // Check if we hit EOF (read fewer than requested)
        if (words_read_count < (int)words_capacity || _reader->is_eof()) {
            _eof_reached = true;
        }

        return _limit >= n;
    }

    void refill() {
        _pos = 0;
        _limit = 0;
        _eof_reached = false;
        ensure_available(1); // Force a read
    }
};

// =========================================================================
// Verification Logic
// =========================================================================

bool check_file_sorted_sparse(
    const std::string& filename,
    uint64_t bits_per_color,
    bool verbose_flag
) {
    uint64_t idx = 0;
    try {
        BufferedPageReader reader(filename, bits_per_color);
        
        Element prev;
        if (!reader.next(prev)) {
            if (verbose_flag) std::cout << "File is empty.\n";
            return true;
        }

        
        Element curr;

        while (reader.next(curr)) {
            // Check: prev > curr ?
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

    if (verbose_flag) {
        std::cout << "File " << filename << " is correctly sorted (" << idx << " elements).\n";
    }
    return true;
}

// =========================================================================
// Phase 1: Create Sorted Chunks
// =========================================================================

void write_chunk(const std::vector<Element>& batch, const std::string& filename) {
    // Use library factory
    std::unique_ptr<stream_writer> writer(stream_writer_library::allocate(filename));
    if (!writer || !writer->is_open()) {
        throw std::runtime_error("Cannot open output chunk: " + filename);
    }

    for (const auto& el : batch) {
        // Write Minimizer
        writer->write((void*)&el.minimizer, sizeof(uint64_t), 1);
        // Write Payload
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
    
    // Main reader shared among threads via lock
    BufferedPageReader reader(infile, bits_per_color);
    omp_lock_t read_lock;
    omp_init_lock(&read_lock);

    std::atomic<int> chunk_counter{0};
    bool done = false;

    // Split RAM budget per thread roughly to avoid OOM
    uint64_t ram_per_thread = std::max((uint64_t)2000, max_words_in_RAM / num_threads);

    #pragma omp parallel num_threads(num_threads) shared(done, chunk_files)
    {
        while (true) {
            std::vector<Element> batch;
            // Heuristic reservation
            batch.reserve(ram_per_thread / 4); 

            // --- Critical Section: Read Batch ---
            omp_set_lock(&read_lock);
            if (done) {
                omp_unset_lock(&read_lock);
                break;
            }
            
            size_t words = reader.fill_batch(batch, ram_per_thread);
            if (batch.empty()) {
                done = true;
                omp_unset_lock(&read_lock);
                break;
            }
            omp_unset_lock(&read_lock);
            // ------------------------------------

            // --- Parallel Section: Sort & Write ---
            std::sort(batch.begin(), batch.end(), ElementCmp());

            int id = chunk_counter.fetch_add(1);
            // Use .lz4 extension so the writer library detects compression
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

    // Min-heap: smallest element at top (operator > is used by priority_queue for min behavior)
    bool operator>(const MergeNode& other) const {
        return el.payload > other.el.payload;
    }
};

void nway_merge_sparse(
    const std::vector<std::string>& chunk_files,
    const std::string& outfile,
    int bits_per_color,
    int verbose,
    uint64_t ram_budget_bytes
) {
    size_t n_files = chunk_files.size();
    if (n_files == 0) return;

    if (verbose >= 3) std::cout << "[III] Merging " << n_files << " chunks.\n";

    // 1. Initialize Readers
    // Divide RAM budget among input buffers (leaving some for output overhead)
    size_t buffer_size_per_file = (ram_budget_bytes / (n_files + 1)); 
    if (buffer_size_per_file < 4096) buffer_size_per_file = 4096; // 4KB min

    std::vector<std::unique_ptr<BufferedPageReader>> readers;
    readers.reserve(n_files);

    for (const auto& f : chunk_files) {
        readers.push_back(std::make_unique<BufferedPageReader>(f, bits_per_color, buffer_size_per_file));
    }

    // 2. Initialize Priority Queue
    std::priority_queue<MergeNode, std::vector<MergeNode>, std::greater<MergeNode>> pq;

    for (int i = 0; i < n_files; ++i) {
        Element el;
        if (readers[i]->next(el)) {
            pq.push({std::move(el), i});
        }
    }

    // 3. Output Writer
    std::unique_ptr<stream_writer> writer(stream_writer_library::allocate(outfile));
    if (!writer || !writer->is_open()) {
        throw std::runtime_error("Cannot open output file: " + outfile);
    }

    // 4. Merge Loop
    while (!pq.empty()) {
        // Get smallest
        MergeNode node = pq.top();
        pq.pop();

        // Write to output
        writer->write((void*)&node.el.minimizer, sizeof(uint64_t), 1);
        writer->write((void*)node.el.payload.data(), sizeof(uint64_t), node.el.payload.size());

        // Refill from the specific stream that provided the element
        if (readers[node.stream_idx]->next(node.el)) {
            pq.push(std::move(node)); // Move back into PQ with new data
        }
    }
    
    writer->close();
}

// =========================================================================
// Main Entry Point
// =========================================================================

void external_sort_sparse(
    const std::string& infile,
    const std::string& outfile,
    const std::string& tmp_dir,
    const uint64_t n_colors,
    const uint64_t ram_value_MB,
    const bool keep_tmp_files,
    const int verbose,
    int n_threads
) {
    // 1. Setup Parameters
    uint64_t bits_per_color = std::ceil(std::log2(n_colors));
    if (bits_per_color <= 8) bits_per_color = 8;
    else if (bits_per_color <= 16) bits_per_color = 16;
    else if (bits_per_color <= 32) bits_per_color = 32;
    else bits_per_color = 64;

    const uint64_t bytes_in_RAM = ram_value_MB * 1024ULL * 1024ULL;
    const uint64_t max_words_in_RAM = bytes_in_RAM / sizeof(uint64_t);

    if (verbose >= 3) {
        std::cout << "[III] Sparse External Sort:\n"
                  << "      Bits/Color: " << bits_per_color << "\n"
                  << "      RAM Limit: " << ram_value_MB << " MB\n"
                  << "      Threads: " << n_threads << "\n";
    }

    // 2. Phase 1: Create Sorted Chunks
    // The reader will handle LZ4 decompression automatically based on file extension
    auto chunk_files = parallel_create_chunks(infile, tmp_dir, max_words_in_RAM, bits_per_color, verbose, n_threads);

    // 3. Phase 2: Merge or Rename
    if (chunk_files.empty()) {
        // Input was empty, create empty output
        std::unique_ptr<stream_writer> w(stream_writer_library::allocate(outfile));
        w->close();
    } else if (chunk_files.size() == 1) {
        std::filesystem::rename(chunk_files[0], outfile);
        if (verbose >= 3) std::cout << "[III] Single chunk renamed to output.\n";
    } else {
        nway_merge_sparse(chunk_files, outfile, bits_per_color, verbose, bytes_in_RAM);
    }

    // 4. Cleanup
    if (!keep_tmp_files) {
        for (const auto& f : chunk_files) {
            std::filesystem::remove(f);
        }
    }
}