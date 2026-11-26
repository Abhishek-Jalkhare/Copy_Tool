#include <atomic>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>

#include "../Copy_Tool.h"

//
// Created by AbhishekJalkhare on 23-11-2026.
//
void write (Params& obj )
{
    // //const std::string& dstnPath, std::queue<std::vector<char>>& buffer,
    // std::mutex& mtx  , std::condition_variable& not_empty, std::condition_variable& not_full
    // ,const bool& done_reading , std::
    // atomic<bool>& stop_read
    auto& [dstnPath, rbuffer ,fbuffer ,
        mtx, not_empty, not_full, done_reading, stop_read] = obj;
    struct Guard
    {
        std::mutex& mtx;
        std::condition_variable& not_full;
        std::atomic<bool>& stop_reading;
        ~Guard()
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                stop_reading = true;
            }
            not_full.notify_all();
        }
    } guard{mtx , not_full , stop_read};
    std::ofstream dst;
    dst.rdbuf()->pubsetbuf(nullptr, 0);   // unbuffered; must be called BEFORE open()
    dst.open(dstnPath, std::ios::binary);
    if (!dst.is_open())
    {
        throw std::runtime_error("Unable to open file for writing: " + dstnPath);
    }
    Chunk chunk;
    while (true){
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [&]()
        {
            return done_reading || !rbuffer.empty();
        });
        if (done_reading && rbuffer.empty())
        {
            break;
        }
        chunk = std::move(rbuffer.front());
        rbuffer.pop();
        lock.unlock();
        dst.write(chunk.data.data() , chunk.size);
        if (!dst)
        {
            throw std::runtime_error("I/O error while writing file");
        }
        {
            std::lock_guard lck(mtx);
            fbuffer.push(std::move(chunk));
        }
        not_full.notify_one();
    }
    dst.flush();
    if (!dst)
    {
        throw std::runtime_error("failed to flush the file");
    }

}
