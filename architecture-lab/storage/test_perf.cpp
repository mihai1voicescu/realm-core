/*************************************************************************
 *
 * Copyright 2016 Realm Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>

#include <chrono>
#include <cassert>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <list>
#include <unordered_map>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "db.hpp"

#define CHUNK_SIZE 10

struct chunk {
    uint16_t symbols[CHUNK_SIZE];
    int prefix_index = -1;
    bool operator==(const chunk& a2) const
    {
        return memcmp(symbols, a2.symbols, 2 * CHUNK_SIZE) == 0 && prefix_index == a2.prefix_index;
    };
};

template <>
struct std::hash<chunk> {
    std::size_t operator()(chunk const& c) const noexcept
    {
        std::size_t ret = (uint64_t)c.symbols[0] << 48;
        ret ^= (uint64_t)c.symbols[1] << 32;
        ret ^= (uint64_t)c.symbols[2] << 16;
        ret ^= (uint64_t)c.symbols[3];
        ret ^= (uint64_t)c.symbols[4] << 48;
        ret ^= (uint64_t)c.symbols[5] << 32;
        ret ^= (uint64_t)c.symbols[6] << 16;
        ret ^= (uint64_t)c.symbols[7];
        ret ^= (uint64_t)c.symbols[8] << 48;
        ret ^= (uint64_t)c.symbols[9] << 32;
#if 0
        std::size_t ret = (unsigned long)c.chars[0] << 56;
        ret ^= (unsigned long)c.chars[1] << 48;
        ret ^= (unsigned long)c.chars[2] << 40;
        ret ^= (unsigned long)c.chars[3] << 32;
#if (CHUNK_SIZE >= 12)
        ret ^= (unsigned long)c.chars[4] << 24;
        ret ^= (unsigned long)c.chars[5] << 16;
        ret ^= (unsigned long)c.chars[6] << 8;
        ret ^= (unsigned long)c.chars[7];
        ret ^= (unsigned long)c.chars[8] << 56;
        ret ^= (unsigned long)c.chars[9] << 48;
        ret ^= (unsigned long)c.chars[10] << 40;
        ret ^= (unsigned long)c.chars[11] << 32;
#endif
#if (CHUNK_SIZE >= 20)
        ret ^= (unsigned long)c.chars[12] << 24;
        ret ^= (unsigned long)c.chars[13] << 16;
        ret ^= (unsigned long)c.chars[14] << 8;
        ret ^= (unsigned long)c.chars[15];
        ret ^= (unsigned long)c.chars[16] << 56;
        ret ^= (unsigned long)c.chars[17] << 48;
        ret ^= (unsigned long)c.chars[18] << 40;
        ret ^= (unsigned long)c.chars[19] << 32;
#endif
#endif
        ret ^= (unsigned long)c.prefix_index;
        // std::cout << " *" << ret << "* " << std::flush;
        return ret;
    }
};

struct encoding_entry {
    uint32_t expansion;
    uint16_t symbol;
    bool valid = false;
};

int hash(uint32_t expansion)
{
    // range must match size of encoding table
    uint32_t tmp = (expansion >> 16) + 3;
    tmp *= expansion + 7;
    return (tmp ^ (tmp >> 16)) & 0xFFFF;
}

class string_compressor {
public:
    std::vector<chunk> chunks;
    std::unordered_map<chunk, int> map;
    std::vector<encoding_entry> encoding_table;
    std::vector<uint32_t> decoding_table;
    string_compressor()
    {
        encoding_table.resize(65536);
    }
    int compress_symbols(uint16_t symbols[], int size)
    {
        // std::cout << "compress ";
        // for (int i = 0; i < size; ++i)
        //     std::cout << symbols[i] << " ";
        for (int runs = 0; runs < 5; ++runs) {
            uint16_t* from = symbols;
            uint16_t* to = symbols;
            int p;
            for (p = 0; p < size - 1; p += 2) {
                uint32_t pair = (from[0] << 16) | from[1];
                auto index = hash(pair);
                auto& e = encoding_table[index];
                if (e.valid && e.expansion == pair) {
                    // existing matching entry
                    *to++ = e.symbol;
                }
                else if (e.valid || decoding_table.size() >= 65536 - 256) {
                    // existing conflicting entry or at capacity
                    *to++ = from[0];
                    *to++ = from[1];
                }
                else {
                    // no entry yet, creating new one
                    e.symbol = decoding_table.size() + 256;
                    decoding_table.push_back(pair);
                    e.expansion = pair;
                    e.valid = true;
                    *to++ = e.symbol;
                }

                /*

                                    auto it = encoding_table.find(pair);
                                if (it == encoding_table.end()) {
                                    bool learning = encoding_table.size() < 65535 - 256;
                                    if (learning) {
                                        uint16_t symbol = encoding_table.size() + 256;
                                        encoding_table[pair] = symbol;
                                        decoding_table.push_back(pair);
                                        *to++ = symbol;
                                    }
                                    else {
                                        // retain old symbols
                                        *to++ = from[0];
                                        *to++ = from[1];
                                    }
                                }
                                else {
                                    *to++ = it->second;
                                }
                                */
                from += 2;
            }
            if (p < size) {
                *to++ = *from;
            }
            *to = 0;
            size = to - symbols;
            if (from == to)
                break; // early out, no change
            //    std::cout << std::endl << " ---> ";
            //    for (int i = 0; i < size; ++i)
            //        std::cout << symbols[i] << " ";
            //    std::cout << std::endl;
        }
        return size;
    }
    void decompress_and_verify(uint16_t symbols[], int size, const char* first, const char* past)
    {
        uint16_t decompressed[8192];
        uint16_t* from = symbols;
        uint16_t* to = decompressed;

        auto decompress = [&](uint16_t symbol, auto& recurse) -> void {
            if (symbol < 256)
                *to++ = symbol;
            else {
                auto expansion = decoding_table[symbol - 256];
                recurse(expansion >> 16, recurse);
                recurse(expansion & 0x0FFFF, recurse);
            }
        };

        while (size--) {
            decompress(*from++, decompress);
        }
        // walk back on any trailing zeroes:
        while (to[-1] == 0 && to > decompressed) {
            --to;
        }
        size = to - decompressed;
        // std::cout << "reverse -> ";
        // for (int i = 0; i < size; ++i) {
        //    std::cout << decompressed[i] << " ";
        // }
        // std::cout << std::endl;
        assert(size == past - first);
        uint16_t* checked = decompressed;
        while (first < past) {
            assert((0xFF & *first++) == *checked++);
        }
    }
    int handle(const char* _first, const char* _past)
    {
        // expand into 16 bit symbols:
        int size = _past - _first;
        total_chars += size;
        assert(size < 8180);
        uint16_t symbols[8192];
        uint16_t* to = symbols;
        for (const char* p = _first; p < _past; ++p)
            *to++ = *p & 0xFF;
        *to++ = 0;
        size = compress_symbols(symbols, size);
        // decompress_and_verify(symbols, size, _first, _past);
        uint16_t* first = symbols;
        uint16_t* past = symbols + size;
        uint16_t* last = first + CHUNK_SIZE;
        int prefix = -1;
        chunk c;
        while (first < past) {
            if (last >= past) {
                last = past;
                memset(c.symbols, 0, 2 * CHUNK_SIZE);
            }
            memcpy(c.symbols, first, 2 * (last - first));
            c.prefix_index = prefix;
            auto it = map.find(c);
            if (it == map.end()) {
                prefix = chunks.size();
                map[c] = prefix;
                chunks.push_back(c);
            }
            else {
                prefix = it->second;
            }
            first += CHUNK_SIZE;
            last += CHUNK_SIZE;
        }
        return prefix;
    }
    int symbol_table_size()
    {
        return decoding_table.size();
    }
    int next_id = 0;
    int next_prefix_id = -1;
    int64_t total_chars = 0;
};

struct compressor_driver {
    struct entry {
        long* res;
        const char* first;
        const char* last;
    };
    std::vector<entry> work;
    string_compressor& compressor;
    compressor_driver(string_compressor& compressor, long size)
        : compressor(compressor)
    {
        work.reserve(size);
    }
    void add_to_work(long* res, const char* first, const char* last)
    {
        work.push_back({res, first, last});
    }
    void perform()
    {
        for (auto& entry : work) {
            *entry.res = compressor.handle(entry.first, entry.last);
        }
        work.clear();
    }
};

struct results {
    long* values;
    long first_line = -1;
    long num_lines;
    long num_fields;
    results(long num_lines, long num_fields)
        : num_lines(num_lines)
        , num_fields(num_fields)
    {
        values = new long[num_lines * num_fields];
    }
    ~results()
    {
        delete values;
    }
    void finalize(long _first_line, long limit)
    {
        first_line = _first_line;
        num_lines = limit - _first_line;
    };
};

template <typename work_type>
class concurrent_queue {
    std::list<work_type> queue;
    std::mutex mutex;
    std::condition_variable changed;
    bool open = true;

public:
    void close()
    {
        std::unique_lock<std::mutex> lock(mutex);
        open = false;
        changed.notify_all();
    }
    void put(work_type work_item)
    {
        std::unique_lock<std::mutex> lock(mutex);
        queue.push_back(work_item);
        changed.notify_all();
    }
    work_type get()
    { // throws if closed and queue empty
        std::unique_lock<std::mutex> lock(mutex);
        while (queue.size() == 0 && open) {
            changed.wait(lock);
        }
        if (queue.size() > 0) {
            auto e = queue.front();
            queue.pop_front();
            return e;
        }
        if (!open) {
            throw std::runtime_error("Concurrent queue closed");
        }
        exit(-3);
    }
};

int main(int argc, char* argv[])
{
    const int limit = 1000000;
    constexpr int max_fields = 105;
    char compressible[max_fields + 1] =
        "iisissiiiiiiissiiiiiiiiiisiiisiiiissiiisiiiiisiiiisiiiiisiiiiiissiiiiiiiiissiiiiiiiiiisississssssssssiiii";

    char fields[max_fields + 1] =
        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii";
    /*
    {
        std::cout << "Determining field types...." << std::endl;
        std::ifstream infile(argv[1]);
        std::string line;
        long num_line = 0;
        while (std::getline(infile, line)) {
            long num_value = 0;
            if ((num_line % 100000) == 0)
                std::cout << num_line << " " << std::flush;
            std::istringstream iss(line);
            ++num_line;
            std::string tmp;
            while (std::getline(iss, tmp, '\t')) {
                //std::cout << num_line << " : " << num_value << " : " << tmp << std::endl;
                if (fields[num_value] == 'i') {
                    // confirm that we can ingest this field as an integer
                    for (int i = 0; i < tmp.length(); ++i) {
                        if (i == 0 && tmp[i] == '-') continue;
                        if (tmp[i] >= '0' && tmp[i] <= '9') continue;
                        fields[num_value] = 's';
                        break;
                    }
                }
                ++num_value;
            }
        }
        std::cout << "Fields: " << fields << std::endl;
    }
    */
    Db& db = Db::create("perf.core2");

    Snapshot& ss = db.create_changes();
    Table t = ss.create_table(fields, 100000000);
    Field<String> f_s[max_fields];
    Field<int64_t> f_i[max_fields];
    for (int j = 0; j < max_fields; ++j) {
        if (fields[j] == 'i')
            f_i[j] = ss.get_field<int64_t>(t, j);
        else
            f_s[j] = ss.get_field<String>(t, j);
    }
    int fd = open(argv[1], O_RDONLY);
    assert(fd >= 0);
    off_t size = lseek(fd, 0, SEEK_END);
    void* file_start = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(file_start != (void*)-1);
    long step_size = 5000000;
    int num_work_packages = 12;
    concurrent_queue<results*> to_reader;
    for (int i = 0; i < num_work_packages; ++i)
        to_reader.put(new results(step_size, max_fields));
    concurrent_queue<results*> to_writer;

    std::thread writer([&]() {
        std::cout << "Initial scan / object creation" << std::endl;
        std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
        long total_lines = 0;
        start = std::chrono::high_resolution_clock::now();
        const char* line_start = (const char*)file_start;
        while (line_start < file_start + size) {
            while (*line_start++ != '\n')
                ;
            if ((total_lines % 100000) == 0)
                std::cout << "." << std::flush;
            if ((total_lines % 1000000) == 0)
                std::cout << total_lines << std::endl;
            ss.insert(t, {total_lines << 1});
            // if (!ss.exists(t, {total_lines << 1})) {
            //     std::cout << "Weird - " << total_lines << " is missing" << std::endl;
            // }
            // auto o = ss.get(t, {total_lines << 1});
            total_lines++;
        }
        end = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "   ...done in " << ms.count() << " millisecs" << std::endl;
        ss.print_stat(std::cout);
        std::cout << "Committing to stable storage" << std::flush;
        start = std::chrono::high_resolution_clock::now();
        db.commit(std::move(ss));
        end = std::chrono::high_resolution_clock::now();
        ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "   ...done in " << ms.count() << " msecs" << std::endl << std::endl;


        std::cout << "Optimizing access order..." << std::endl;
        std::vector<Row> row_order;
        row_order.reserve(total_lines);
        {
            const Snapshot& s3 = db.open_snapshot();
            s3.for_each(t, [&](Object& o) {
                row_order.push_back(o.r); /* std::cout << o.r.key << " " << std::flush; */
            });
            db.release(std::move(s3));
        }
        std::cout << "Committing data...." << std::endl;
        while (1) {

            results* res;
            try {
                res = to_writer.get();
            }
            catch (...) {
                std::cout << "Done" << std::endl;
                break;
            }

            start = std::chrono::high_resolution_clock::now();
            const Snapshot& s3 = db.open_snapshot();
            db.release(std::move(s3));
            Snapshot& s2 = db.create_changes(); // create_changes();
            auto num_line = res->first_line;
            auto limit = res->first_line + res->num_lines;
            std::cout << "Writing " << num_line << " to " << limit << " width " << res->num_fields << std::endl;
            auto write_object = [&](long num_line, long*& val_ptr) {
                auto row = row_order[num_line];
                Object o = s2.get(t, row);
                auto num_value = 0;
                while (num_value < res->num_fields) {
                    auto val = *val_ptr++;
                    o.set(f_i[num_value++], val);
                }
            };
            auto write_range = [&](long first, long past) {
                std::cout << "constructing [" << first << " - " << past << "[" << std::endl;
                auto val_ptr = res->values + (first - res->first_line) * res->num_fields;
                for (auto line = first; line < past; ++line) {
                    write_object(line, val_ptr);
                }
            };
            // split in 5 chunks and guard them against races by writing 500 entries
            // at the borders of each of the chunks.
            auto step = 1000000; // res->num_lines / 5;
            // 4 separating zone:
            for (auto line = num_line + step; line < limit; line += step) {
                write_range(line, line + 500);
            }
            // write 5 much larger in-between ranges in parallel
            std::vector<std::unique_ptr<std::thread>> threads;
            threads.push_back(std::make_unique<std::thread>([&]() { write_range(num_line, num_line + step); }));
            threads.push_back(
                std::make_unique<std::thread>([&]() { write_range(num_line + step + 500, num_line + 2 * step); }));
            threads.push_back(std::make_unique<std::thread>(
                [&]() { write_range(num_line + 2 * step + 500, num_line + 3 * step); }));
            threads.push_back(std::make_unique<std::thread>(
                [&]() { write_range(num_line + 3 * step + 500, num_line + 4 * step); }));
            write_range(num_line + 4 * step + 500, limit);
            for (auto& p : threads) {
                if (p)
                    p->join();
            }
            to_reader.put(res);
            end = std::chrono::high_resolution_clock::now();
            std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "   ...transaction built in " << ms.count() << " millisecs" << std::endl;
            start = end;
            db.commit(std::move(s2));
            end = std::chrono::high_resolution_clock::now();
            ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "   ...committed in " << ms.count() << " msecs" << std::endl << std::endl;
        }
    });
    // now populate dataset
    {
        std::cout << std::endl << "Ingesting data.... " << std::endl;
        std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
        long num_line = 0;
        std::vector<std::unique_ptr<string_compressor>> compressors;
        compressors.resize(105);
        std::vector<std::unique_ptr<compressor_driver>> drivers;
        drivers.resize(105);
        const char* read_ptr = (const char*)file_start;
        while (read_ptr < file_start + size) {
            long limit = num_line + step_size;
            long first_line = num_line;
            results* res = to_reader.get();
            start = std::chrono::high_resolution_clock::now();
            while (num_line < limit && read_ptr < file_start + size) {
                long num_value = 0;
                if ((num_line % 100000) == 0)
                    std::cout << num_line << " " << std::flush;
                // std::cout << "Index " << num_line << " - key " << row.key << std::endl;
                long* line_results = res->values + (num_line - first_line) * max_fields;
                ++num_line;
                while (num_value < max_fields) {
                    const char* read_ptr2 = read_ptr;
                    while (*read_ptr2 != '\t' && *read_ptr2 != 0 && *read_ptr2 != '\n')
                        ++read_ptr2;
                    if (compressible[num_value] == 's') {
                        if (!compressors[num_value]) {
                            compressors[num_value] = std::make_unique<string_compressor>();
                            drivers[num_value] =
                                std::make_unique<compressor_driver>(*compressors[num_value], 1000000);
                        }
                        auto& compressor = *compressors[num_value];
                        // long compressed = compressor.handle(read_ptr, read_ptr2);
                        // line_results[num_value] = compressed;
                        drivers[num_value]->add_to_work(line_results + num_value, read_ptr, read_ptr2);
                    }
                    else if (read_ptr == read_ptr2)
                        line_results[num_value] = 0;
                    else
                        line_results[num_value] = atol(read_ptr);

                    ++num_value;
                    read_ptr = read_ptr2 + 1;
                }
            }
            res->finalize(first_line, num_line);
            end = std::chrono::high_resolution_clock::now();
            std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << std::endl << "   ...read in " << ms.count() << " millisecs" << std::endl;
            start = end;
            {
                std::vector<std::unique_ptr<std::thread>> threads;
                for (auto& p : drivers) {
                    if (p) {
                        // p->perform();
                        threads.push_back(std::make_unique<std::thread>([&]() { p->perform(); }));
                    }
                }
                for (auto& p : threads) {
                    p->join();
                }
            }
            to_writer.put(res);
            end = std::chrono::high_resolution_clock::now();
            ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "   ...compressed in " << ms.count() << " millisecs" << std::endl;
        }
        std::cout << "shutting down..." << std::endl;
        to_writer.close();

        uint64_t from_size = 0;
        uint64_t to_size = 0;
        for (int i = 0; i < max_fields; ++i) {
            if (compressors[i]) {
                from_size += compressors[i]->total_chars;
                to_size += compressors[i]->map.size() * sizeof(chunk);
                std::cout << "Field " << i << " with " << compressors[i]->map.size() << " chunks ("
                          << compressors[i]->map.size() * sizeof(chunk) << " bytes) from total "
                          << compressors[i]->total_chars << " chars"
                          << " (symbol table: " << compressors[i]->symbol_table_size() << " )" << std::endl;
                compressors[i].reset();
            }
        }
        std::cout << "Total effect: from " << from_size << " to " << to_size << " bytes ("
                  << 100 - (to_size * 100) / from_size << " pct reduction)" << std::endl;

        compressors.clear();
        for (int i = 0; i < num_work_packages; ++i)
            delete to_reader.get();

        writer.join();
    }
}
