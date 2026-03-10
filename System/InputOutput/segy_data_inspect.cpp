#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "segy_data_interface.h"

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3)
    {
        std::cerr << "usage: segy_data_inspect <input.segy> [max_samples_to_print]" << std::endl;
        return 1;
    }

    const int max_samples_to_print = (argc == 3) ? std::max(1, std::atoi(argv[2])) : 10;

    try
    {
        SegyDataInterface segy(argv[1]);
        std::vector<float> data = segy.read_all_traces();

        const std::size_t samples_per_trace = segy.binary_header().num_samples_per_trace;
        const std::size_t trace_count = segy.trace_count();

        if (data.empty() || samples_per_trace == 0 || trace_count == 0)
        {
            std::cerr << "error: empty SEG-Y data" << std::endl;
            return 2;
        }

        float trace_min = data[0];
        float trace_max = data[0];
        for (std::size_t i = 1; i < samples_per_trace; ++i)
        {
            trace_min = std::min(trace_min, data[i]);
            trace_max = std::max(trace_max, data[i]);
        }

        std::cout << "SEG-Y header summary" << std::endl;
        std::cout << "trace_count=" << trace_count
                  << " samples_per_trace=" << samples_per_trace
                  << " sample_interval_us=" << segy.binary_header().sample_interval_us
                  << " sample_format=" << segy.binary_header().data_sample_format
                  << std::endl;
        std::cout << "first_trace_min=" << trace_min << " first_trace_max=" << trace_max << std::endl;
        std::cout << "first_trace_samples:" << std::endl;
        for (int i = 0; i < max_samples_to_print && i < static_cast<int>(samples_per_trace); ++i)
        {
            std::cout << "  [" << i << "] " << data[static_cast<std::size_t>(i)] << std::endl;
        }

        return 0;
    }
    catch (std::exception const &ex)
    {
        std::cerr << "error: " << ex.what() << std::endl;
        return 10;
    }
}
