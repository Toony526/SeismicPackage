#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "segy_data_interface.h"

namespace
{
std::string remove_extension(std::string const &path)
{
    std::string::size_type slash = path.find_last_of("/\\");
    std::string::size_type dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
    {
        return path;
    }
    return path.substr(0, dot);
}

void print_usage()
{
    std::cerr << "usage: segy_to_sep <input.segy> [output.header output.trace] [traces_per_chunk] [decode_threads]" << std::endl;
    std::cerr << "example: segy_to_sep shot.sgy shot.header shot.trace 4096 8" << std::endl;
}
} // namespace

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 4 && argc != 5 && argc != 6)
    {
        print_usage();
        return 1;
    }

    std::string input_path = argv[1];
    std::string header_path;
    std::string trace_path;

    int arg_index = 2;
    if (argc >= 4)
    {
        header_path = argv[2];
        trace_path = argv[3];
        arg_index = 4;
    }
    else
    {
        std::string stem = remove_extension(input_path);
        header_path = stem + ".header";
        trace_path = stem + ".trace";
    }

    SegyDataInterface::ConvertOptions options;
    if (arg_index < argc)
    {
        options.traces_per_chunk = static_cast<std::size_t>(std::strtoul(argv[arg_index], NULL, 10));
        ++arg_index;
    }
    if (arg_index < argc)
    {
        options.decode_threads = static_cast<std::size_t>(std::strtoul(argv[arg_index], NULL, 10));
    }

    try
    {
        SegyDataInterface interface(input_path);
        interface.write_as_sep(header_path, trace_path, options);

        std::cout << "Converted SEG-Y to SEP." << std::endl;
        std::cout << "trace_count=" << interface.trace_count()
                  << " samples_per_trace=" << interface.binary_header().num_samples_per_trace
                  << " sample_interval_us=" << interface.binary_header().sample_interval_us
                  << std::endl;
        std::cout << "output_header=" << header_path << std::endl;
        std::cout << "output_trace=" << trace_path << std::endl;
        std::cout << "Internal format for workflow: .header + .trace" << std::endl;
    }
    catch (std::exception const &ex)
    {
        std::cerr << "error: " << ex.what() << std::endl;
        return 2;
    }

    return 0;
}
