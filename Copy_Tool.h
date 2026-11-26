//
// Created by AbhishekJalkhare on 25-11-2026.
//
#include <string>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>
#include <condition_variable>


#ifndef COPY_TOOL_COPY_TOOL_H
#define COPY_TOOL_COPY_TOOL_H

struct Chunk
{
    std::vector<char> data;
    std::streamsize size = 0;
};


struct Params
{
    const std::string& srcPath;
    std::queue<Chunk>& rbuffer;
    std::queue<Chunk>& fbuffer;
    std::mutex& mtx;
    std::condition_variable& not_empty;
    std::condition_variable& not_full;
    bool& done_reading;
    std::atomic<bool>& stop_read;

};

struct parser_params
{
    const std::string srcPath;
    const std::string dstPath;
    const std::streamsize chunk_size;
    const std::size_t buffers;      // number of chunks circulating in the pipeline
    const bool force;
};

void read(Params& obj );

void write (Params& obj);

void validate(const parser_params& params);

std::optional<parser_params> parse(int argc, char* argv[]);

void copy_file(const parser_params& params );

#endif //COPY_TOOL_COPY_TOOL_H
