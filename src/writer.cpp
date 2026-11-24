#include <atomic>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>

namespace std
{
    class mutex;
    class condition_variable;
}

//
// Created by AbhishekJalkhare on 23-11-2026.
//
void write ( const std::string& dstnPath, std::queue<std::vector<char>>& buffer,
    std::mutex& mtx  , std::condition_variable& not_empty, std::condition_variable& not_full
    ,const bool& done_reading , std::
    atomic<bool>& stop_read  )
{
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
    std::ofstream dst(dstnPath , std::ios::binary);
    if (!dst.is_open())
    {
        throw std::runtime_error("Unable to open file for writing");
    }
    std::vector<char> chunk;
    while (true){
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [&]()
        {
            return done_reading || !buffer.empty();
        });
        if (done_reading && buffer.empty())
        {
            break;
        }
        chunk = std::move(buffer.front());
        buffer.pop();
        lock.unlock();

        dst.write(chunk.data() , chunk.size());
        if (!dst)
        {
            throw std::runtime_error("I/O error while writing file");
        }

    }
    dst.flush();
    if (!dst)
    {
        throw std::runtime_error("failed to flush the file");
    }

}
