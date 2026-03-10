#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "sep_reader.h"
#include "segy_data_interface.h"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "usage: segy_sep_validate <input.segy> <output.header>" << std::endl;
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
        std::size_t finite_count = 0;
        std::size_t non_finite_count = 0;

        for (std::size_t i = 0; i < segy_data.size(); ++i)
        {
            const float a = segy_data[i];
            const float b = sep_data[i];

            if (!std::isfinite(a) || !std::isfinite(b))
            {
                ++non_finite_count;
                continue;
            }

            const float diff = std::fabs(a - b);
            if (!std::isfinite(diff))
            {
                ++non_finite_count;
                continue;
            }

            if (diff > max_abs_diff)
            {
                max_abs_diff = diff;
            }
            mse += static_cast<double>(diff) * static_cast<double>(diff);
            ++finite_count;
        }

        if (finite_count == 0)
        {
            std::cerr << "validation failed: no finite samples available for RMSE calculation" << std::endl;
            return 4;
        }

        const double rmse = std::sqrt(mse / static_cast<double>(finite_count));

        std::cout << "Validation finished." << std::endl;
        std::cout << "trace_count=" << segy.trace_count()
                  << " samples_per_trace=" << segy.binary_header().num_samples_per_trace
                  << " sample_interval_us=" << segy.binary_header().sample_interval_us
                  << std::endl;
        std::cout << "max_abs_diff=" << max_abs_diff
                  << " rmse=" << rmse
                  << " finite_samples=" << finite_count
                  << " non_finite_samples=" << non_finite_count
                  << std::endl;

        if (non_finite_count > 0)
        {
            std::cerr << "validation warning: encountered non-finite samples; conversion cannot be treated as strictly identical" << std::endl;
            return 5;
        }

        std::cout << "Validation passed." << std::endl;
        return 0;
    }
    catch (std::exception const &ex)
    {
        std::cerr << "error: " << ex.what() << std::endl;
        return 10;
    }
}
