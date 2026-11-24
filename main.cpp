#include <algorithm>
#include <condition_variable>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include<atomic>
#include <utility>
#include <boost/program_options.hpp>
#include<filesystem>
namespace po = boost::program_options;
namespace fs = std::filesystem;


void read(const std::string& srcPath, std::queue<std::vector<char>>& buffer,
          std::mutex& mtx  , std::condition_variable& not_empty, std::condition_variable& not_full,
          bool& done_reading ,const std::atomic<bool>& stop_read , const std::streamsize chunkSize , const size_t max_queue_size );

void write ( const std::string& dstnPath, std::queue<std::vector<char>>& buffer,
    std::mutex& mtx  , std::condition_variable& not_empty, std::condition_variable& not_full
    ,const bool& done_reading , std::
    atomic<bool>& stop_read  );

void storeError (std::mutex& mtx , std::exception_ptr& error , std::exception_ptr e)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (!error) error = std::move(e);
}

int main(int argc, char* argv[]){
    po::options_description visible_options("Options Description");
    bool force;
    visible_options.add_options()
        ("help,h", "Display the help message")
        ("chunk_size,c",po::value<std::streamsize>()->default_value(1024) , "size of the chunks will be read or written at a time")
        ("force,f" , po::bool_switch(&force),"if used replace the content of destination file if present");

    po::options_description hidden_options("Hidden Options");
    hidden_options.add_options()
        ("src" , po::value<std::string>()->required(), "src file name")
        ("dst" , po::value<std::string>()->required(), "dst file name");

    po::options_description all_options("All Options");
    all_options.
        add(visible_options).
        add(hidden_options);

    po::positional_options_description pos;
    pos.add("src", 1);
    pos.add("dst", 1);
    po::variables_map vm;
    try
    {
        po::store(po::command_line_parser(argc, argv).
            options(all_options).
            positional(pos).
            run(), vm);
    }catch (std::exception& e)
    {
        std::cout<<"Argument Error: "<<e.what()<< std::endl;
        return 1;
    }

    if (vm.count("help"))
    {
        std::cout<<visible_options<<std::endl;
        return 0;
    }

    try{
        po::notify(vm);
    }catch (std::exception& e)
    {
        std::cout<<"Argument Error: "<<e.what()<< std::endl;
        return 1;
    }



    auto srcPath = vm["src"].as<std::string>();
    auto dstPath = vm["dst"].as<std::string>();
    const std::streamsize chunk_size = vm["chunk_size"].as<std::streamsize>();
    const size_t max_queue_size = chunk_size*16;
    const fs::path dstFile = dstPath;
    const fs::path srcFile = srcPath;
    std::exception_ptr error;
    if (chunk_size < 0)
    {
        std::cerr<<"Chunk size must be greater than zero\n";
        return 1;
    }

    if (!fs::is_regular_file(srcPath))
    {
        std::cerr<<"File dose not exist \n";
    }

    if (fs::exists(dstPath))
    {
        if (!force)
        {
            std::cerr << "Destination exists (use -f to overwrite): " << dstPath << '\n';
            return 1;
        }
        if (fs::equivalent(srcPath, dstPath))
        {
            std::cerr << "Source and destination are the same file\n";
            return 1;
        }
    }


    {
        std::atomic<bool> stop_reading(false);
        std::condition_variable not_empty;
        std::condition_variable not_full;
        std::queue<std::vector<char>> buffer;
        std::mutex mtx;
        bool done_reading = false;
        std::jthread t1([&]
       {
           try
           {
               read(srcPath, buffer, mtx, not_empty,not_full, done_reading, stop_reading,chunk_size , max_queue_size);
           }catch (...)
           {
               storeError(mtx,error,std::current_exception());
           }
       });


        std::jthread t2([&]
        {
            try
            {
                write(dstPath, buffer, mtx, not_empty , not_full , done_reading, stop_reading);
            }catch (...)
            {
                storeError(mtx,error,std::current_exception());
            }
        });
    }

    if (error)
    {
        try
        {
            std::rethrow_exception(error);
        }
        catch (const std::exception& e)
        {
            std::cerr<<"Copy failed: " << e.what() << '\n';
        }
        catch (...)
        {
            std::cerr<<"Unknown exception occured\n";
        }
        return 1;
    }

    std::cout<<"Copying Completed"<<std::endl;
    return 0;

}


