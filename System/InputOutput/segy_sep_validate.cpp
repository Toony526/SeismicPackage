#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "sep_reader.h"
#include "segy_data_interface.h"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "usage: segy_sep_validate <input.segy> <output.H>" << std::endl;
        return 1;
    }

    try
    {
        SegyDataInterface segy(argv[1]);
        std::vector<float> segy_data = segy.read_all_traces();

        SEPReader sep(argv[2]);
        const std::size_t sep_samples_per_trace = static_cast<std::size_t>(sep.n1);
        const std::size_t sep_trace_count = static_cast<std::size_t>(sep.n2) *
                                            static_cast<std::size_t>(sep.n3) *
                                            static_cast<std::size_t>(sep.n4) *
                                            static_cast<std::size_t>(sep.n5) *
                                            static_cast<std::size_t>(sep.n6) *
                                            static_cast<std::size_t>(sep.n7) *
                                            static_cast<std::size_t>(sep.n8);

        if (sep_samples_per_trace != segy.binary_header().num_samples_per_trace)
        {
            std::cerr << "sample count mismatch: segy="
                      << segy.binary_header().num_samples_per_trace
                      << " sep=" << sep_samples_per_trace << std::endl;
            return 2;
        }

        if (sep_trace_count != segy.trace_count())
        {
            std::cerr << "trace count mismatch: segy=" << segy.trace_count()
                      << " sep=" << sep_trace_count << std::endl;
            return 3;
        }

        std::vector<float> sep_data(segy_data.size(), 0.0f);
        sep.read_sepval(sep_data.data(), sep.o1, sep.o2, sep.o3, static_cast<int>(sep_data.size()));

        float max_abs_diff = 0.0f;
        double mse = 0.0;

        for (std::size_t i = 0; i < segy_data.size(); ++i)
        {
            const float diff = std::fabs(segy_data[i] - sep_data[i]);
            if (diff > max_abs_diff)
            {
                max_abs_diff = diff;
            }
            mse += static_cast<double>(diff) * static_cast<double>(diff);
        }

        mse /= static_cast<double>(segy_data.size());
        const double rmse = std::sqrt(mse);

        std::cout << "Validation passed." << std::endl;
        std::cout << "trace_count=" << segy.trace_count()
                  << " samples_per_trace=" << segy.binary_header().num_samples_per_trace
                  << " sample_interval_us=" << segy.binary_header().sample_interval_us
                  << std::endl;
        std::cout << "max_abs_diff=" << max_abs_diff << " rmse=" << rmse << std::endl;

        return 0;
    }
    catch (std::exception const &ex)
    {
        std::cerr << "error: " << ex.what() << std::endl;
        return 10;
    }
}
