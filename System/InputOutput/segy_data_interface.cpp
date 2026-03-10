#include "segy_data_interface.h"

#include <fstream>
#include <stdexcept>
#include <vector>

#include "sep_writer.h"

namespace
{
std::uint16_t read_be_u16(std::istream &input)
{
    unsigned char bytes[2] = {0, 0};
    input.read(reinterpret_cast<char *>(bytes), 2);
    if (!input)
    {
        throw std::runtime_error("failed to read uint16 from segy");
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[0]) << 8) |
                                      static_cast<std::uint16_t>(bytes[1]));
}

std::uint32_t read_be_u32(std::istream &input)
{
    unsigned char bytes[4] = {0, 0, 0, 0};
    input.read(reinterpret_cast<char *>(bytes), 4);
    if (!input)
    {
        throw std::runtime_error("failed to read uint32 from segy");
    }
    return (static_cast<std::uint32_t>(bytes[0]) << 24) |
           (static_cast<std::uint32_t>(bytes[1]) << 16) |
           (static_cast<std::uint32_t>(bytes[2]) << 8) |
            static_cast<std::uint32_t>(bytes[3]);
}

float be_u32_to_ieee_float(std::uint32_t value)
{
    union
    {
        std::uint32_t i;
        float f;
    } converter;
    converter.i = value;
    return converter.f;
}
} // namespace

SegyDataInterface::SegyDataInterface(const std::string &file_path)
    : file_path_(file_path)
{
    parse_headers();
}

SegyDataInterface::BinaryHeader const & SegyDataInterface::binary_header() const
{
    return binary_header_;
}

std::size_t SegyDataInterface::trace_count() const
{
    return trace_count_;
}

std::size_t SegyDataInterface::bytes_per_sample() const
{
    switch (binary_header_.data_sample_format)
    {
    case 5:
        return 4; // IEEE float
    default:
        throw std::runtime_error("unsupported SEG-Y sample format; currently only format 5 (IEEE float) is supported");
    }
}

void SegyDataInterface::parse_headers()
{
    std::ifstream input(file_path_, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("failed to open SEG-Y file: " + file_path_);
    }

    input.seekg(3200 + 16, std::ios::beg);
    binary_header_.sample_interval_us = read_be_u16(input);
    binary_header_.num_samples_per_trace = read_be_u16(input);

    input.seekg(3200 + 24, std::ios::beg);
    binary_header_.data_sample_format = read_be_u16(input);

    input.seekg(0, std::ios::end);
    const std::streamoff file_size = input.tellg();
    const std::streamoff data_start = 3600;
    const std::streamoff trace_size = 240 +
        static_cast<std::streamoff>(binary_header_.num_samples_per_trace * bytes_per_sample());

    if (file_size < data_start || trace_size <= 0)
    {
        throw std::runtime_error("invalid SEG-Y file size or header values");
    }

    trace_count_ = static_cast<std::size_t>((file_size - data_start) / trace_size);
    if (trace_count_ == 0)
    {
        throw std::runtime_error("SEG-Y file has no traces");
    }
}

std::vector<float> SegyDataInterface::read_all_traces() const
{
    std::ifstream input(file_path_, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("failed to open SEG-Y file: " + file_path_);
    }

    const std::size_t samples_per_trace = binary_header_.num_samples_per_trace;
    std::vector<float> output(trace_count_ * samples_per_trace, 0.0f);

    input.seekg(3600, std::ios::beg);

    for (std::size_t trace = 0; trace < trace_count_; ++trace)
    {
        input.seekg(240, std::ios::cur); // skip trace header
        for (std::size_t sample = 0; sample < samples_per_trace; ++sample)
        {
            const std::uint32_t be_val = read_be_u32(input);
            output[trace * samples_per_trace + sample] = be_u32_to_ieee_float(be_val);
        }
    }

    return output;
}

void SegyDataInterface::write_as_sep(const std::string &sep_header_path,
                                     const std::string &sep_data_path) const
{
    std::vector<float> traces = read_all_traces();

    std::vector<std::string> header_labels;
    header_labels.push_back("trace_index");

    std::vector<std::string> sort_order;
    sort_order.push_back("trace_index");

    SEPWriter writer(sep_header_path.c_str(),
                     0, static_cast<int>(binary_header_.sample_interval_us), static_cast<int>(binary_header_.num_samples_per_trace),
                     0, 1, static_cast<int>(trace_count_),
                     0, 1, 1,
                     header_labels,
                     sort_order,
                     sep_data_path.c_str());

    writer.OpenDataFile(sep_data_path.c_str());
    writer.write_sepval(traces.data(),
                        0,
                        0,
                        0,
                        static_cast<int>(traces.size()));
}
