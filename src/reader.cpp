//
// Created by AbhishekJalkhare on 23-11-2026.
//

#include <atomic>
#include <fstream>
#include <ios>
#include <mutex>
#include <queue>
#include <string>
#include <boost/program_options/detail/cmdline.hpp>

#include "../Copy_Tool.h"


void read(Params& obj)
{
    auto& [srcPath , rbuffer,fbuffer ,
        mtx , not_empty ,not_full ,
        done_reading , stop_read ] = obj;
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
    std::ifstream src;
    src.rdbuf()->pubsetbuf(nullptr, 0);   // unbuffered; must be called BEFORE open()
    src.open(srcPath, std::ios::binary);
    if (!src.is_open())
    {
        throw std::runtime_error("Unable to open file for reading");
    }

    // A struct that acts as a guard whenever the function throws it sets done to true then exits


    while (src && !stop_read)
    {
        Chunk chunk;
        {
            std::unique_lock<std::mutex> lock(mtx);
            not_full.wait(lock, [&]()
            {
                return  !fbuffer.empty() || stop_read;
            });
            if (stop_read) break;
            chunk = std::move(fbuffer.front());
            fbuffer.pop();
        }
        src.read(chunk.data.data(), chunk.data.size());
        chunk.size = src.gcount();
        if (chunk.size <= 0 ) break;
        {
            std::lock_guard lock(mtx);
            rbuffer.push(std::move(chunk));
        }
        not_empty.notify_one();
    }
    if (src.bad())
    {
        throw std::runtime_error("I/O error while reading file");
    }

}

