//
// Created by AbhishekJalkhare on 25-11-2026.
//
#include <exception>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "../Copy_Tool.h"

void storeError (std::mutex& mtx , std::exception_ptr& error , const std::exception_ptr& e)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (!error) error = e;
}

void copy_file(const parser_params& params)
{
    std::queue<Chunk> rbuffer;
    std::queue<Chunk> fbuffer;
    std::mutex mtx;
    std::condition_variable not_empty;
    std::condition_variable not_full;
    bool done_reading = false;
    std::atomic<bool> stop_read = false;

    constexpr size_t kBuffers = 6;
    for (size_t i = 0; i < kBuffers; ++i)
    {
        fbuffer.push(Chunk{ std::vector<char>(params.chunk_size), 0 });
    }
    
    Params w_params{
        params.dstPath,
        rbuffer,
        fbuffer,
        mtx,
        not_empty,
        not_full,
        done_reading,
        stop_read,
    };

    Params r_params{
        params.srcPath,
        rbuffer,
        fbuffer,
        mtx,
        not_empty,
        not_full,
        done_reading,
        stop_read,
    };

    std::exception_ptr e;
    std::jthread t1([&]()
    {
        try
        {
            read(r_params);
        }catch (...)
        {
            storeError(w_params.mtx,e,std::current_exception());
        }
    });

    std::jthread t2([&]()
    {
        try
        {
            write(w_params);
        }catch (...)
        {
            storeError(w_params.mtx,e,std::current_exception());
        }
    });

    t1.join();
    t2.join();

    if (e) rethrow_exception(e);
}