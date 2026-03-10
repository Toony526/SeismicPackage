#include <iostream>
#include <stdexcept>

#include "segy_data_interface.h"

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        std::cerr << "usage: segy_to_sep <input.segy> <output.H> <output.@>" << std::endl;
        return 1;
    }

    try
    {
        SegyDataInterface interface(argv[1]);
        interface.write_as_sep(argv[2], argv[3]);

        std::cout << "Converted SEG-Y to SEP." << std::endl;
        std::cout << "trace_count=" << interface.trace_count()
                  << " samples_per_trace=" << interface.binary_header().num_samples_per_trace
                  << " sample_interval_us=" << interface.binary_header().sample_interval_us
                  << std::endl;
        std::cout << "Recommended internal format for this repository: SEP (.H + .@)" << std::endl;
    }
    catch (std::exception const &ex)
    {
        std::cerr << "error: " << ex.what() << std::endl;
        return 2;
    }

    return 0;
}
