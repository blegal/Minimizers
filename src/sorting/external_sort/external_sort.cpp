#include "external_sort.hpp"
#include "../../../lib/BreiZHMinimizer.hpp"
#include "../../kmer_list/smer_deduplication.hpp"

// Wrapper Includes
#include "../../files/stream_reader_library.hpp"
#include "../../files/stream_writer_library.hpp"
#include "../../../include/config.hpp"

// Standard Includes
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstring>
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <iostream>

// ------------------------------------------------------------
// Thread-Safe Queue for Producer-Consumer Pattern
// ------------------------------------------------------------
struct Job {
    std::vector<uint64_t> data; // The raw data chunk
    size_t id;                  // Chunk ID for naming the file
};

template <typename T>
class SafeQueue {
private:
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv_producer;
    std::condition_variable cv_consumer;
    size_t max_size;
    bool stop_flag = false;

public:
    SafeQueue(size_t max_items) : max_size(max_items) {}

    void push(T item) {
        std::unique_lock<std::mutex> lock(m);
        // Wait if queue is full (Backpressure to prevent RAM explosion)
        cv_producer.wait(lock, [this] { return q.size() < max_size; });
        q.push(std::move(item));
        cv_consumer.notify_one();
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(m);
        cv_consumer.wait(lock, [this] { return !q.empty() || stop_flag; });

        if (q.empty() && stop_flag) return false; // Done

        item = std::move(q.front());
        q.pop();
        cv_producer.notify_one();
        return true;
    }

    void stop() {
        std::unique_lock<std::mutex> lock(m);
        stop_flag = true;
        cv_consumer.notify_all();
    }
};

// ------------------------------------------------------------
// Utility Functions (Updated with Wrappers)
// ------------------------------------------------------------
void generate_random_example(const std::string& filename,
                             double size_in_MB,
                             uint64_t num_colors,
                             bool verbose = true) {
    uint64_t n_uint_color = (num_colors + 63) / 64; 
    uint64_t n_uint_per_element = 1 + n_uint_color; 

    uint64_t target_bytes = static_cast<uint64_t>(size_in_MB * 1024.0 * 1024.0);
    uint64_t elem_size = n_uint_per_element * sizeof(uint64_t);
    uint64_t num_elements = target_bytes / elem_size;

    if (num_elements == 0)
        throw std::runtime_error("Requested size too small to contain one element");

    if (verbose) {
        std::cout << "Generating file: " << filename << "\n";
        std::cout << "  Elements: " << num_elements << "\n";
    }

    // WRAPPER: Allocate writer
    stream_writer* writer = stream_writer_library::allocate(filename);
    if (!writer || !writer->is_open()) {
        if(writer) delete writer;
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }

    std::mt19937_64 rng(42); 
    std::uniform_int_distribution<uint64_t> dist_minimizer(0, UINT64_MAX);
    std::uniform_int_distribution<uint64_t> dist_bits(0, UINT64_MAX);

    std::vector<uint64_t> buffer(n_uint_per_element * 4096); 
    size_t written = 0;

    while (written < num_elements) {
        size_t block_elems = std::min<size_t>(4096, num_elements - written);
        for (size_t i = 0; i < block_elems; i++) {
            size_t base = i * n_uint_per_element;
            buffer[base] = dist_minimizer(rng); 
            for (uint64_t j = 1; j < n_uint_per_element; j++) {
                buffer[base + j] = dist_bits(rng); 
            }
        }
        // WRAPPER: Write
        writer->write(buffer.data(), elem_size, block_elems);
        written += block_elems;
    }

    writer->close();
    delete writer;

    if (verbose) std::cout << "Done generating.\n";
}

bool check_file_sorted(
    const std::string& filename,
    uint64_t n_uint_per_element,
    bool verbose_flag
) {
    if (n_uint_per_element < 2) return false;

    // WRAPPER: Allocate reader
    stream_reader* reader = stream_reader_library::allocate(filename);
    if (!reader || !reader->is_open()) {
        if(reader) delete reader;
        std::cout << "Error: cannot open " << filename << "\n";
        return false;
    }

    const size_t bytes_per_element = n_uint_per_element * sizeof(uint64_t);
    const size_t color_bytes = (n_uint_per_element - 1) * sizeof(uint64_t);

    const size_t target_bytes = 1 << 20; // 1 MiB buffer
    size_t elems_per_buf = std::max<size_t>(1, target_bytes / bytes_per_element);
    std::vector<uint64_t> buffer(elems_per_buf * n_uint_per_element);

    std::vector<uint64_t> prev_color(n_uint_per_element - 1);
    bool have_prev = false;
    uint64_t element_index = 0;

    while (true) {
        // WRAPPER: Read
        int got = reader->read(buffer.data(), bytes_per_element, elems_per_buf);
        if (got <= 0) break;

        for (int i = 0; i < got; ++i) {
            uint64_t* elem_ptr = buffer.data() + i * n_uint_per_element;
            uint64_t* color_ptr = elem_ptr + 1; // skip minimizer

            if (have_prev) {
                if (memcmp(prev_color.data(), color_ptr, color_bytes) > 0) {
                    std::cout << "File not sorted at index " << element_index << "\n";
                    delete reader;
                    return false;
                }
            } else {
                have_prev = true;
            }
            memcpy(prev_color.data(), color_ptr, color_bytes);
            ++element_index;
        }
    }

    delete reader;
    if (verbose_flag) std::cout << "File is sorted (" << element_index << " elements).\n";
    return true;
}

// ------------------------------------------------------------
// Structs for Merge Phase
// ------------------------------------------------------------
struct ChunkEntry {
    std::vector<uint64_t> buffer;
    size_t pos = 0;
    size_t count = 0;
    stream_reader* reader = nullptr; // WRAPPER: replaced FILE*
};

struct EntryCmp {
    const uint64_t n_uint_per_element;
    EntryCmp(uint64_t n) : n_uint_per_element(n) {}
    bool operator()(const std::pair<uint64_t*, size_t>& a,
                    const std::pair<uint64_t*, size_t>& b) const {
        // Sort primarily by color payload (skipping first uint64)
        return memcmp(a.first + 1, b.first + 1,
                      (n_uint_per_element - 1) * sizeof(uint64_t)) > 0;
    }
};

// ------------------------------------------------------------
// PHASE 1: Parallel Chunk Creation (Producer-Consumer)
// ------------------------------------------------------------
std::vector<std::string> create_chunks_parallel(const std::string& infile,
                                                const std::string& tmp_dir,
                                                uint64_t n_uint_per_element,
                                                uint64_t max_elements_in_RAM,
                                                uint64_t bytes_per_element,
                                                int n_threads,
                                                int verbose)
{
    std::vector<std::string> chunknames;
    std::mutex chunks_mutex; // Protects 'chunknames'
    
    // Split RAM: 1 part for Producer, rest split among Worker threads + queue slots
    // n_workers = threads available for sorting/writing
    size_t n_workers = std::max(1, n_threads - 1);
    
    // We size the chunks such that (Queue Size + Active Workers) * ChunkSize <= RAM
    // Safety factor: Queue depth = n_workers.
    // Total slots in flight = n_workers (processing) + n_workers (queued) + 1 (reading)
    uint64_t elements_per_chunk = max_elements_in_RAM / (2 * n_workers + 1);
    
    // Fallback if chunks are too small
    if (elements_per_chunk == 0) elements_per_chunk = 1000;

    SafeQueue<Job> queue(n_workers);

    if (verbose >= 3) {
        std::cout << "[III] Producer-Consumer: 1 Reader + " << n_workers << " Sorters.\n";
        std::cout << "[III] Elements per chunk: " << elements_per_chunk << "\n";
    }

    // --- WORKER THREAD FUNCTION ---
    auto worker_task = [&]() {
        Job job;
        while (queue.pop(job)) {
            size_t n_items = job.data.size() / n_uint_per_element;
            
            // 1. Setup Pointers for Sorting
            std::vector<uint64_t*> ptrs(n_items);
            for (size_t i = 0; i < n_items; i++) {
                ptrs[i] = &job.data[i * n_uint_per_element];
            }

            // 2. Sort (CPU Bound)
            auto cmp = [&](const uint64_t* a, const uint64_t* b) {
                return memcmp(a + 1, b + 1, (n_uint_per_element - 1) * sizeof(uint64_t)) < 0;
            };
            std::sort(ptrs.begin(), ptrs.end(), cmp);

            // 3. Write (I/O Bound)
            std::string fname = tmp_dir + "/chunk_" + std::to_string(job.id) + ".bin";
            
            stream_writer* writer = stream_writer_library::allocate(fname);
            if(writer && writer->is_open()) {
                // Writing element-by-element since ptrs are shuffled
                for (size_t i = 0; i < n_items; i++) {
                    writer->write(ptrs[i], bytes_per_element, 1);
                }
                writer->close();
                delete writer;

                // Register the chunk
                std::lock_guard<std::mutex> lock(chunks_mutex);
                chunknames.push_back(fname);
            } else {
                if(writer) delete writer;
                std::cerr << "Error writing chunk: " << fname << "\n";
            }
        }
    };

    // Start Workers
    std::vector<std::thread> threads;
    for(int i=0; i < (int)n_workers; ++i) {
        threads.emplace_back(worker_task);
    }

    // --- PRODUCER (Main Thread) ---
    // WRAPPER: Allocate reader
    stream_reader* reader = stream_reader_library::allocate(infile);
    if (!reader || !reader->is_open()) {
        if(reader) delete reader;
        queue.stop();
        for(auto& t : threads) if(t.joinable()) t.join();
        throw std::runtime_error("Cannot open input: " + infile);
    }

    size_t chunk_id = 0;
    while (true) {
        Job job;
        job.data.resize(elements_per_chunk * n_uint_per_element);
        job.id = chunk_id;

        // READ sequential stream
        int got = reader->read(job.data.data(), bytes_per_element, elements_per_chunk);
        
        if (got <= 0) break; // EOF

        // Resize to actual count
        if ((uint64_t)got < elements_per_chunk) {
            job.data.resize(got * n_uint_per_element);
        }

        // Push to queue (Blocks if queue is full)
        queue.push(std::move(job));
        chunk_id++;
    }

    delete reader;
    
    // Shutdown
    queue.stop();
    for (auto& t : threads) if(t.joinable()) t.join();

    return chunknames;
}

// ------------------------------------------------------------
// PHASE 2: N-Way Merge (Sequential Output)
// ------------------------------------------------------------
void nway_merge(const std::vector<std::string>& chunknames,
                const std::string& outfile,
                uint64_t n_uint_per_element,
                uint64_t max_elements_in_RAM,
                int verbose) 
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    size_t n_chunks = chunknames.size();

    // Allocate RAM evenly
    uint64_t buf_elems = max_elements_in_RAM / (n_chunks + 1);
    if (buf_elems == 0)
        throw std::runtime_error("Not enough RAM to allocate buffers for merge.");

    std::vector<uint64_t> outbuf(buf_elems * n_uint_per_element);
    std::vector<ChunkEntry> chunks(n_chunks);

    // Initialize Chunks
    for (size_t i = 0; i < n_chunks; i++) {
        chunks[i].buffer.resize(buf_elems * n_uint_per_element);
        
        // WRAPPER: Allocate reader per chunk
        chunks[i].reader = stream_reader_library::allocate(chunknames[i]);
        if (!chunks[i].reader || !chunks[i].reader->is_open())
            throw std::runtime_error("Cannot open chunk: " + chunknames[i]);
            
        chunks[i].count = chunks[i].reader->read(chunks[i].buffer.data(),
                                                 n_uint_per_element * sizeof(uint64_t),
                                                 buf_elems);
        chunks[i].pos = 0;
    }

    // WRAPPER: Allocate output writer
    stream_writer* writer = stream_writer_library::allocate(outfile);
    if (!writer || !writer->is_open())
        throw std::runtime_error("Cannot open output: " + outfile);

    // Min-Heap
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

        // Buffer Output
        memcpy(&outbuf[outcount * n_uint_per_element],
               ptr,
               n_uint_per_element * sizeof(uint64_t));
        outcount++;

        if (outcount == buf_elems) {
            writer->write(outbuf.data(),
                          n_uint_per_element * sizeof(uint64_t),
                          buf_elems);
            outcount = 0;
        }

        // Refill Chunk
        chunks[idx].pos++;
        if (chunks[idx].pos == chunks[idx].count) {
            chunks[idx].count = chunks[idx].reader->read(chunks[idx].buffer.data(),
                                                         n_uint_per_element * sizeof(uint64_t),
                                                         buf_elems);
            chunks[idx].pos = 0;
            if (chunks[idx].count <= 0) {
                // EOF for this chunk
                delete chunks[idx].reader;
                chunks[idx].reader = nullptr;
                continue;
            }
        }

        heap.push_back({ chunks[idx].buffer.data() + chunks[idx].pos * n_uint_per_element, idx });
        std::push_heap(heap.begin(), heap.end(), cmp);
    }

    // Flush remaining
    if (outcount > 0) {
        writer->write(outbuf.data(),
                      n_uint_per_element * sizeof(uint64_t),
                      outcount);
    }

    writer->close();
    delete writer;

    // Safety cleanup
    for(size_t i=0; i<n_chunks; i++){
        if(chunks[i].reader) {
            delete chunks[i].reader;
            chunks[i].reader = nullptr;
        }
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    if (verbose >= 3) {
        std::cout << "[III] N-way merge took: " << elapsed.count() << " seconds.\n";
    }
}

// ------------------------------------------------------------
// Multi-Pass Logic
// ------------------------------------------------------------
void multi_pass_nway_merge(
    std::vector<std::string> chunknames,
    const std::string& outfile,
    uint64_t n_uint_per_element,
    uint64_t max_elements_in_RAM,
    int verbose = 0) 
{
    if (chunknames.empty())
        throw std::runtime_error("multi_pass_nway_merge: empty chunk list");

    size_t pass_num = 0;
    // Conservative fan-in to avoid running out of file descriptors
    const size_t FAN_IN = 64; 

    while (chunknames.size() > 1) {
        pass_num++;
        size_t n_chunks = chunknames.size();
        size_t n_groups = (n_chunks + FAN_IN - 1) / FAN_IN;

        std::vector<std::string> tmp_files(n_groups);
        
        for (size_t g = 0; g < n_groups; ++g) {
            size_t start_idx = g * FAN_IN;
            size_t end_idx = std::min(start_idx + FAN_IN, n_chunks);
            std::vector<std::string> group_chunks(chunknames.begin() + start_idx,
                                                  chunknames.begin() + end_idx);

            tmp_files[g] = outfile + "_tmp_pass" + std::to_string(pass_num) + "_group" + std::to_string(g) + ".bin";

            if (verbose >= 3) {
                std::cout << "[III] Pass " << pass_num << ": Group " << g 
                          << " (" << group_chunks.size() << " chunks) -> " << tmp_files[g] << "\n";
            }

            nway_merge(group_chunks, tmp_files[g], n_uint_per_element, max_elements_in_RAM, verbose);
        }

        // Delete previous pass chunks
        for (const auto& f : chunknames) {
            std::filesystem::remove(f);
        }

        chunknames = std::move(tmp_files);
    }

    // Final Rename
    if (!chunknames.empty() && chunknames[0] != outfile) {
        std::filesystem::rename(chunknames[0], outfile);
    }
}

// ------------------------------------------------------------
// Main External Sort Entry Point
// ------------------------------------------------------------
void external_sort( const std::string& infile,
                    const std::string& outfile,
                    const std::string& tmp_dir,
                    const uint64_t n_colors,
                    const uint64_t ram_value_MB,
                    const bool keep_tmp_files,
                    const int verbose,
                    int n_threads)
{
    if (verbose >= 3) {
        std::cout << "[III] Starting External Sort (Streaming + Producer-Consumer) on: " << infile << std::endl;
    }

    const uint64_t n_uint_per_element = (n_colors + 63) / 64 + 1;
    const uint64_t bytes_per_element  = n_uint_per_element * sizeof(uint64_t);
    uint64_t max_elements_in_RAM      = static_cast<uint64_t>((ram_value_MB * 1024 * 1024) / bytes_per_element);

    // Phase 1: Create Chunks using Producer-Consumer
    auto chunknames = create_chunks_parallel(infile, tmp_dir,
                                             n_uint_per_element,
                                             max_elements_in_RAM,
                                             bytes_per_element,
                                             n_threads,
                                             verbose);

    if (chunknames.size() == 1) {
        std::filesystem::rename(chunknames[0], outfile);
        return;
    }

    // Phase 2: Merge
    multi_pass_nway_merge(chunknames, outfile,
                         n_uint_per_element,
                         max_elements_in_RAM,
                         verbose);

    // Cleanup input if requested
    if (!keep_tmp_files) {
        std::remove(infile.c_str());
    }
}