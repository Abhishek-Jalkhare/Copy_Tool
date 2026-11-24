//
// Created by AbhishekJalkhare on 23-11-2026.
//

#include <atomic>
#include <fstream>
#include <ios>
#include <mutex>
#include <queue>
#include <string>

namespace std
{
    class mutex;
    class condition_variable;
}

void read(const std::string& srcPath, std::queue<std::vector<char>>& buffer,
          std::mutex& mtx  , std::condition_variable& not_empty, std::condition_variable& not_full,
          bool& done_reading ,const std::atomic<bool>& stop_read , const std::streamsize chunkSize , const size_t max_queue_size )
{
    struct Guard
    {
        std::mutex& mtx;
        std::condition_variable& not_empty;
        bool& done_reading;
        ~Guard()
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                done_reading = true;
            }
            not_empty.notify_all();
        }

    } guard(mtx, not_empty, done_reading);
    std::ifstream src(srcPath , std::ios::binary);
    if (!src.is_open())
    {
        throw std::runtime_error("Unable to open file for reading");
    }

    // A struct that acts as a guard whenever the function throws it sets done to true then exits



    while (src && !stop_read)
    {
        std::vector<char> chunk(chunkSize);
        src.read(chunk.data(), chunkSize);
        const std::streamsize bytesRead = src.gcount();
        if (bytesRead <= 0 ) break;
        if (bytesRead != chunkSize)
        {
            chunk.resize(bytesRead);
        }
        {
            std::unique_lock<std::mutex> lock(mtx);
            not_full.wait(lock, [&]()
            {
                return buffer.size() < max_queue_size || stop_read;
            });
            if (stop_read) break;
            buffer.push(std::move(chunk));
        }
        not_empty.notify_one();
    }
    if (src.bad())
    {
        throw std::runtime_error("I/O error while reading file");
    }

}

