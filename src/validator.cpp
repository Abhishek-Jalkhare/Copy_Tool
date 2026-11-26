//
// Created by AbhishekJalkhare on 25-11-2026.
//

#include <filesystem>
#include <system_error>

#include "../Copy_Tool.h"

void validate(const parser_params& params)
{
    std::error_code ec; // to use the the error_code overloads of fs methods which does
                        // not throw any exception because we will throw our exception with
                        // custom msg

    if (params.chunk_size <= 0 )
        throw std::invalid_argument("chunk_size must be greater than 0.");

    if (!std::filesystem::is_regular_file(params.srcPath, ec))
        throw std::invalid_argument("srcPath Error : " + ec.message());

    if (std::filesystem::exists(params.dstPath, ec))
    {
        if (std::filesystem::equivalent(params.srcPath, params.dstPath, ec))
            throw std::invalid_argument("Source and destination are the same file");
        if (!params.force)
            throw std::invalid_argument("Destination exists (use -f to overwrite): " + params.dstPath);
    }
}