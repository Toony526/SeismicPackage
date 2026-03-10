#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>

#include "sep_reader.h"

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3)
    {
        std::cerr << "usage: sep_data_inspect <input.H> [max_samples_to_print]" << std::endl;
        return 1;
    }

    const int max_samples_to_print = (argc == 3) ? std::max(1, std::atoi(argv[2])) : 10;

    SEPReader reader(argv[1]);

    const std::size_t samples_per_trace = static_cast<std::size_t>(reader.n1);
    const std::size_t trace_count = static_cast<std::size_t>(reader.n2) *
                                    static_cast<std::size_t>(reader.n3) *
                                    static_cast<std::size_t>(reader.n4) *
                                    static_cast<std::size_t>(reader.n5) *
                                    static_cast<std::size_t>(reader.n6) *
                                    static_cast<std::size_t>(reader.n7) *
                                    static_cast<std::size_t>(reader.n8);

    std::vector<float> first_trace(samples_per_trace, 0.0f);
    reader.read_sepval(first_trace.data(), reader.o1, reader.o2, reader.o3, static_cast<int>(samples_per_trace));

    float trace_min = first_trace[0];
    float trace_max = first_trace[0];
    for (std::size_t i = 1; i < first_trace.size(); ++i)
    {
        trace_min = std::min(trace_min, first_trace[i]);
        trace_max = std::max(trace_max, first_trace[i]);
    }

    std::cout << "SEP header summary" << std::endl;
    std::cout << "n1=" << reader.n1 << " n2=" << reader.n2 << " n3=" << reader.n3
              << " n4=" << reader.n4 << " n5=" << reader.n5 << " n6=" << reader.n6
              << " n7=" << reader.n7 << " n8=" << reader.n8 << std::endl;
    std::cout << "o1=" << reader.o1 << " d1=" << reader.d1
              << " trace_count=" << trace_count << std::endl;

    std::cout << "header_labels:";
    for (std::size_t i = 0; i < reader.get_header_labels().size(); ++i)
    {
        std::cout << " " << reader.get_header_labels()[i];
    }
    std::cout << std::endl;

    std::cout << "sort_order:";
    for (std::size_t i = 0; i < reader.get_sort_order().size(); ++i)
    {
        std::cout << " " << reader.get_sort_order()[i];
    }
    std::cout << std::endl;

    std::cout << "first_trace_min=" << trace_min << " first_trace_max=" << trace_max << std::endl;
    std::cout << "first_trace_samples:" << std::endl;
    for (int i = 0; i < max_samples_to_print && i < static_cast<int>(first_trace.size()); ++i)
    {
        std::cout << "  [" << i << "] " << first_trace[static_cast<std::size_t>(i)] << std::endl;
    }

    return 0;
}
