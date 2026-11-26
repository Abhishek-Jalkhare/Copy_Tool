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
#include "Copy_Tool.h"
namespace po = boost::program_options;
namespace fs = std::filesystem;


int main(int argc, char* argv[]){
    try
    {
        const auto params =  parse(argc , argv);
        if (!params)
        {
            return 0; //-h was passed
        }

        validate(*params);
        //const auto start = std::chrono::steady_clock::now();
        copy_file(*params);
        // const auto end = std::chrono::steady_clock::now();
        // const std::chrono::duration<double> elapsed = end - start;
        // std::cout<<elapsed.count()<<" seconds"<<std::endl;


    }catch( std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Error: unknown exception\n";
        return 1;
    }
    //std::cout<<"Copying Completed"<<std::endl;
    return 0;

}


