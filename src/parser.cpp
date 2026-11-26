//
// Created by AbhishekJalkhare on 25-11-2026.
//
#include "../Copy_Tool.h"
#include <boost/program_options.hpp>
#include <cstddef>
#include <iostream>
#include <string>

namespace po = boost::program_options;

std::optional<parser_params> parse(int argc, char* argv[])
{
    po::options_description visible_options("Options Description");
    visible_options.add_options()
        ("help,h", "Display the help message")
        ("chunk_size,c", po::value<std::streamsize>()->default_value(4 << 20),
            "size in bytes of the chunks read or written at a time")
        ("buffers,b", po::value<std::size_t>()->default_value(6),
            "number of chunk buffers circulating between reader and writer (min 2)")
        ("force,f", po::bool_switch(), "if used replace the content of destination file if present");

    po::options_description hidden_options("Hidden Options");
    hidden_options.add_options()
        ("src", po::value<std::string>()->required(), "src file name")
        ("dst", po::value<std::string>()->required(), "dst file name");

    po::options_description all_options("All Options");
    all_options.add(visible_options).add(hidden_options);

    po::positional_options_description pos;
    pos.add("src", 1);
    pos.add("dst", 1);
    po::variables_map vm;

    po::store(po::command_line_parser(argc, argv)
        .options(all_options)
        .positional(pos)
        .run(), vm);

    if (vm.count("help"))
    {
        std::cout << visible_options << std::endl;
        return std::nullopt;
    }

    po::notify(vm);

    return parser_params{
        vm["src"].as<std::string>(),
        vm["dst"].as<std::string>(),
        vm["chunk_size"].as<std::streamsize>(),
        vm["buffers"].as<std::size_t>(),
        vm["force"].as<bool>()
    };
}