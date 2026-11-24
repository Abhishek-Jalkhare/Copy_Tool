cmake_minimum_required(VERSION 4.3)
project(Copy_Tool)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(Copy_Tool main.cpp
        src/reader.cpp
        src/writer.cpp)
target_link_libraries(Copy_Tool PRIVATE Boost::program_options)