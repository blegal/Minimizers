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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
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
// Comparators
// ------------------------------------------------------------

// Comparator for Min-Heap (Returns true if A > B)
// Used in Merge Phase to keep Smallest element at the root (std::greater logic)
struct ElementCmpGreater {
    const uint64_t n_uint_per_element;
    ElementCmpGreater(uint64_t n) : n_uint_per_element(n) {}

    // Updated signature to accept the Heap's value_type
    bool operator()(const std::pair<uint64_t*, size_t>& a, 
                    const std::pair<uint64_t*, size_t>& b) const {
        
        const uint64_t* ptrA = a.first;
        const uint64_t* ptrB = b.first;

        // 1. Compare Colors (Skip minimizer at index 0)
        int res = std::memcmp(ptrA + 1, ptrB + 1, (n_uint_per_element - 1) * sizeof(uint64_t));
        
        // 2. Tie-breaker: Compare Minimizer for deterministic output
        if (res == 0) {
            return ptrA[0] > ptrB[0]; 
        }
        return res > 0;
    }
};

// Comparator for Standard Sort (Returns true if A < B)
// Used in Chunk Creation Phase
struct ElementCmpLess {
    const uint64_t n_uint_per_element;
    ElementCmpLess(uint64_t n) : n_uint_per_element(n) {}

    bool operator()(const uint64_t* a, const uint64_t* b) const {
        // 1. Compare Colors
        int res = std::memcmp(a + 1, b + 1, (n_uint_per_element - 1) * sizeof(uint64_t));
        
        // 2. Tie-breaker
        if (res == 0) {
            return a[0] < b[0];
        }
        return res < 0;
    }
};

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
std::string get_extension(const std::string& filename) {
    std::string ext = std::filesystem::path(filename).extension().string();
    if (ext.empty()) return ".bin";
    return ext;
}

// ------------------------------------------------------------
// PHASE 1: Parallel Chunk Creation (Producer-Consumer)
// ------------------------------------------------------------
std::vector<std::string> create_chunks_parallel(const std::string& infile,
                                                const std::string& tmp_dir,
                                                uint64_t n_uint_per_element,
                                                uint64_t max_ram_bytes,
                                                int n_threads,
                                                int verbose,
                                                const std::string& output_extension)
{
    std::vector<std::string> chunknames;
    std::mutex chunks_mutex; 
    
    // Config: 1 Producer, rest Workers
    size_t n_workers = std::max(1, n_threads - 1);
    
    // RAM Calculation: Data + Vector<Pointer> overhead
    uint64_t bytes_per_element_raw = n_uint_per_element * sizeof(uint64_t);
    uint64_t bytes_per_element_ram = bytes_per_element_raw + sizeof(uint64_t*); 

    // Division: (Active Workers + Queue Slots + Producer Buffer)
    // We size chunks so that even if queues are full, we stay under RAM limit.
    uint64_t elements_per_chunk = max_ram_bytes / (bytes_per_element_ram * (2 * n_workers + 1));
    
    // Safety lower bound
    if (elements_per_chunk < 1000) elements_per_chunk = 1000;

    SafeQueue<Job> queue(n_workers);

    if (verbose >= 3) {
        std::cerr << "[III] Producer-Consumer Config:\n"
                  << "      Workers: " << n_workers << "\n"
                  << "      Elems/Chunk: " << elements_per_chunk << "\n"
                  << "      Extension: " << output_extension << "\n";
    }

    // --- WORKER THREAD FUNCTION ---
    auto worker_task = [&]() {
        Job job;
        std::vector<uint64_t*> ptrs; // Avoid reallocating vector container
        ElementCmpLess cmp(n_uint_per_element);
        
        while (queue.pop(job)) {
            size_t n_items = job.data.size() / n_uint_per_element;
            
            // 1. Setup Pointers
            ptrs.resize(n_items);
            for (size_t i = 0; i < n_items; i++) {
                ptrs[i] = &job.data[i * n_uint_per_element];
            }

            // 2. Sort (Standard Sort A < B)
            std::sort(ptrs.begin(), ptrs.end(), cmp);

            // 3. Write
            std::string fname = tmp_dir + "/chunk_" + std::to_string(job.id) + output_extension;
            stream_writer* writer = stream_writer_library::allocate(fname);
            
            if(writer && writer->is_open()) {
                // Optimization: Staging buffer to minimize vtable calls
                const size_t STAGE_COUNT = 4096; 
                std::vector<uint64_t> stage_buf;
                stage_buf.reserve(STAGE_COUNT * n_uint_per_element);

                for (size_t i = 0; i < n_items; i++) {
                    stage_buf.insert(stage_buf.end(), ptrs[i], ptrs[i] + n_uint_per_element);

                    if (stage_buf.size() >= STAGE_COUNT * n_uint_per_element) {
                        writer->write(stage_buf.data(), sizeof(uint64_t) * n_uint_per_element, STAGE_COUNT);
                        stage_buf.clear();
                    }
                }
                // Flush remaining
                if (!stage_buf.empty()) {
                     writer->write(stage_buf.data(), sizeof(uint64_t) * n_uint_per_element, stage_buf.size() / n_uint_per_element);
                }

                writer->close();
                delete writer;

                {
                    std::lock_guard<std::mutex> lock(chunks_mutex);
                    chunknames.push_back(fname);
                }
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
        job.id = chunk_id++;

        int got = reader->read(job.data.data(), sizeof(uint64_t) * n_uint_per_element, elements_per_chunk);
        
        if (got <= 0) break; // EOF

        if ((uint64_t)got < elements_per_chunk) {
            job.data.resize(got * n_uint_per_element);
        }

        queue.push(std::move(job));
    }

    delete reader;
    queue.stop();
    for (auto& t : threads) if(t.joinable()) t.join();

    return chunknames;
}

// ------------------------------------------------------------
// PHASE 2: N-Way Merge
// ------------------------------------------------------------
struct ChunkEntry {
    std::vector<uint64_t> buffer;
    size_t pos = 0;
    size_t count = 0;
    stream_reader* reader = nullptr;
};

void nway_merge(const std::vector<std::string>& chunknames,
                const std::string& outfile,
                uint64_t n_uint_per_element,
                uint64_t max_ram_bytes,
                int verbose) 
{
    size_t n_chunks = chunknames.size();
    if (n_chunks == 0) return;

    // 1. Calculate Buffer Sizes
    uint64_t bytes_per_elem_raw = n_uint_per_element * sizeof(uint64_t);
    
    // Divide RAM by (n_chunks inputs + 1 output)
    uint64_t buf_elems = (max_ram_bytes / bytes_per_elem_raw) / (n_chunks + 1);
    
    // --- CRITICAL FIX: CAP BUFFER SIZE ---
    // The underlying read/write functions use 'int', limiting I/O to ~2GB per call.
    // We cap the buffer at 1GB (1024^3 bytes) to be safe and prevent overflow.
    const uint64_t MAX_IO_BYTES = 1024ULL * 1024ULL * 1024ULL; // 1GB
    
    if (buf_elems * bytes_per_elem_raw > MAX_IO_BYTES) {
        buf_elems = MAX_IO_BYTES / bytes_per_elem_raw;
        // (Optional) Log this if you want to verify the fix
        // if (verbose >= 3) std::cout << "[III] Merge buffers capped to 1GB to prevent IO overflow.\n";
    }

    // Safety floor
    if (buf_elems < 16) buf_elems = 16; 

    std::vector<uint64_t> outbuf(buf_elems * n_uint_per_element);
    std::vector<ChunkEntry> chunks(n_chunks);

    // 2. Initialize Readers (Safe)
    for (size_t i = 0; i < n_chunks; i++) {
        chunks[i].buffer.resize(buf_elems * n_uint_per_element);
        chunks[i].reader = stream_reader_library::allocate(chunknames[i]);
        if (!chunks[i].reader || !chunks[i].reader->is_open())
            throw std::runtime_error("Cannot open chunk: " + chunknames[i]);
            
        // Because buf_elems is capped, this cast is now safe
        int got = chunks[i].reader->read(chunks[i].buffer.data(), bytes_per_elem_raw, (int)buf_elems);
        
        if (got <= 0) {
            chunks[i].count = 0; 
        } else {
            chunks[i].count = static_cast<size_t>(got);
        }
        chunks[i].pos = 0;
    }

    stream_writer* writer = stream_writer_library::allocate(outfile);
    if (!writer || !writer->is_open()) {
        for(auto& c : chunks) if(c.reader) delete c.reader;
        throw std::runtime_error("Cannot open output: " + outfile);
    }

    // 3. Initialize Min-Heap
    std::vector<std::pair<uint64_t*, size_t>> heap;
    ElementCmpGreater cmp(n_uint_per_element);

    for (size_t i = 0; i < n_chunks; i++) {
        if (chunks[i].count > 0) 
            heap.push_back({ chunks[i].buffer.data(), i });
    }
    std::make_heap(heap.begin(), heap.end(), cmp);

    size_t outcount = 0;

    // 4. Merge Loop
    while (!heap.empty()) {
        std::pop_heap(heap.begin(), heap.end(), cmp);
        auto [ptr, idx] = heap.back();
        heap.pop_back();

        // Copy to output
        std::memcpy(&outbuf[outcount * n_uint_per_element], ptr, bytes_per_elem_raw);
        outcount++;

        // Flush Output if full
        if (outcount == buf_elems) {
            // Safe write because outcount <= buf_elems (which is capped)
            writer->write(outbuf.data(), bytes_per_elem_raw, (int)outcount);
            outcount = 0;
        }

        // Advance Reader
        chunks[idx].pos++;
        
        // Refill if needed
        if (chunks[idx].pos == chunks[idx].count) {
            int got = chunks[idx].reader->read(chunks[idx].buffer.data(), bytes_per_elem_raw, (int)buf_elems);
            
            if (got > 0) {
                chunks[idx].count = static_cast<size_t>(got);
                chunks[idx].pos = 0;
            } else {
                chunks[idx].count = 0; // EOF
            }
        }

        // Push back to heap if valid
        if (chunks[idx].pos < chunks[idx].count) {
            heap.push_back({ chunks[idx].buffer.data() + chunks[idx].pos * n_uint_per_element, idx });
            std::push_heap(heap.begin(), heap.end(), cmp);
        } else {
            // Close exhausted chunk
            if (chunks[idx].reader) {
                delete chunks[idx].reader;
                chunks[idx].reader = nullptr;
            }
        }
    }

    // 5. Final Flush
    if (outcount > 0) {
        writer->write(outbuf.data(), bytes_per_elem_raw, (int)outcount);
    }

    writer->close();
    delete writer;

    for(size_t i=0; i<n_chunks; i++) if(chunks[i].reader) delete chunks[i].reader;
}

// ------------------------------------------------------------
// Multi-Pass Logic
// ------------------------------------------------------------
void multi_pass_nway_merge(std::vector<std::string> chunknames,
                           const std::string& outfile,
                           uint64_t n_uint_per_element,
                           uint64_t max_ram_bytes,
                           int verbose) 
{
    const size_t FAN_IN = 64; 
    size_t pass = 0;
    
    // Determine extension for intermediate files
    std::string ext = get_extension(outfile);

    while (chunknames.size() > 1) {
        pass++;
        size_t n_groups = (chunknames.size() + FAN_IN - 1) / FAN_IN;
        std::vector<std::string> next_chunks;

        for (size_t g = 0; g < n_groups; ++g) {
            size_t start = g * FAN_IN;
            size_t end = std::min(start + FAN_IN, chunknames.size());
            
            std::vector<std::string> group;
            group.reserve(end - start);
            for(size_t k=start; k<end; ++k) group.push_back(chunknames[k]);

            std::string tmp_out = outfile + ".tmp.pass" + std::to_string(pass) + ".group" + std::to_string(g) + ext;
            
            if (verbose >= 3) {
                 std::cerr << "[III] Merge Pass " << pass << ", Group " << g 
                           << " (" << group.size() << " files) -> " << tmp_out << "\n";
            }

            nway_merge(group, tmp_out, n_uint_per_element, max_ram_bytes, verbose);
            next_chunks.push_back(tmp_out);

            // Delete merged chunks
            for(const auto& f : group) std::filesystem::remove(f);
        }
        chunknames = next_chunks;
    }

    // Final Rename
    if (!chunknames.empty() && chunknames[0] != outfile) {
        std::filesystem::rename(chunknames[0], outfile);
    }
}

// ------------------------------------------------------------
// Main External Sort Entry Point
// ------------------------------------------------------------
void external_sort(const std::string& infile,
                   const std::string& outfile,
                   const std::string& tmp_dir,
                   const uint64_t n_colors,
                   const uint64_t ram_value_MB,
                   const bool keep_tmp_files,
                   const int verbose,
                   int n_threads)
{
    // Determine output extension (e.g. .lz4) so temporary chunks match format
    std::string out_ext = get_extension(outfile);

    if (verbose >= 3) {
        std::cerr << "[III] Starting External Sort on: " << infile << "\n"
                  << "      Output Extension: " << out_ext << "\n";
    }

    const uint64_t n_uint_per_element = (n_colors + 63) / 64 + 1;
    uint64_t max_ram_bytes = ram_value_MB * 1024 * 1024;

    // Phase 1: Create Chunks using Producer-Consumer
    auto chunknames = create_chunks_parallel(infile, tmp_dir, 
                                             n_uint_per_element, 
                                             max_ram_bytes, 
                                             n_threads, 
                                             verbose,
                                             out_ext);

    if (chunknames.size() == 1) {
        // Single chunk created, just rename it
        std::filesystem::rename(chunknames[0], outfile);
    } else {
        // Phase 2: Merge
        multi_pass_nway_merge(chunknames, outfile, n_uint_per_element, max_ram_bytes, verbose);
    }

    // Cleanup input if requested
    if (!keep_tmp_files) {
        std::filesystem::remove(infile);
    }
}



bool check_file_sorted(const std::string& filename, uint64_t n_colors, bool verbose) {
    // 1. Calculate Element Size
    // Logic must match external_sort: 1 uint64 (minimizer) + N uint64 (colors)
    uint64_t n_uint_per_element = (n_colors + 63) / 64 + 1;
    size_t bytes_per_element = n_uint_per_element * sizeof(uint64_t);
    size_t payload_bytes = (n_uint_per_element - 1) * sizeof(uint64_t);

    if (verbose) {
        std::cerr << "[Check] Verifying file: " << filename << "\n"
                  << "        Uints/Elem: " << n_uint_per_element << " (" << bytes_per_element << " bytes)\n";
    }

    // 2. Open Reader
    stream_reader* reader = stream_reader_library::allocate(filename);
    if (!reader || !reader->is_open()) {
        if (reader) delete reader;
        std::cerr << "[Error] Cannot open file: " << filename << "\n";
        return false;
    }

    // 3. Buffer Setup
    // Read in 1MB chunks to minimize function call overhead
    const size_t BUFFER_SIZE_MB = 1; 
    size_t elems_per_chunk = (BUFFER_SIZE_MB * 1024 * 1024) / bytes_per_element;
    if (elems_per_chunk == 0) elems_per_chunk = 100; // Safety fallback

    std::vector<uint64_t> buffer(elems_per_chunk * n_uint_per_element);
    std::vector<uint64_t> prev_element(n_uint_per_element);
    
    bool first_element_seen = false;
    uint64_t processed_count = 0;

    // 4. Verification Loop
    while (true) {
        // Read a chunk of elements
        int count = reader->read(buffer.data(), bytes_per_element, elems_per_chunk);
        if (count <= 0) break; // EOF

        for (int i = 0; i < count; ++i) {
            uint64_t* curr_ptr = &buffer[i * n_uint_per_element];

            if (first_element_seen) {
                // Compare logic: Must be Prev <= Curr
                // Implementation checks violation: Prev > Curr
                
                // A. Compare Payloads (Colors) - Start at index 1
                int res = std::memcmp(prev_element.data() + 1, curr_ptr + 1, payload_bytes);
                
                bool violation = false;
                if (res > 0) {
                    // Prev Payload > Curr Payload -> Violation
                    violation = true;
                } 
                else if (res == 0) {
                    // Payloads equal, check Minimizer (Tie-breaker)
                    // Prev Minimizer > Curr Minimizer -> Violation
                    if (prev_element[0] > curr_ptr[0]) {
                        violation = true;
                    }
                }

                if (violation) {
                    std::cerr << "[Error] Sorting violation at index " << processed_count << "\n";
                    if (verbose) {
                        std::cerr << "        Prev Min: " << prev_element[0] << "\n"
                                  << "        Curr Min: " << curr_ptr[0] << "\n";
                    }
                    delete reader;
                    return false;
                }
            }

            // Update Previous
            // std::memcpy is faster than vector assignment
            std::memcpy(prev_element.data(), curr_ptr, bytes_per_element);
            
            first_element_seen = true;
            processed_count++;
        }
    }

    delete reader;

    if (verbose) {
        std::cerr << "[Check] Success! " << processed_count << " elements are sorted.\n";
    }
    return true;
}