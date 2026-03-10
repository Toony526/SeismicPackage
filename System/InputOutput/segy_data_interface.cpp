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

std::int16_t be_u16_to_i16(std::uint16_t value)
{
    return static_cast<std::int16_t>(value);
}

std::int32_t be_u32_to_i32(std::uint32_t value)
{
    return static_cast<std::int32_t>(value);
}

float ibm_to_ieee_float(std::uint32_t ibm)
{
    if (ibm == 0)
    {
        return 0.0f;
    }

    const int sign = (ibm & 0x80000000u) ? -1 : 1;
    int exponent = static_cast<int>((ibm >> 24) & 0x7fu) - 64;
    std::uint32_t fraction = ibm & 0x00ffffffu;

    double mantissa = static_cast<double>(fraction) / static_cast<double>(0x01000000u);
    double value = sign * mantissa;

    while (exponent > 0)
    {
        value *= 16.0;
        --exponent;
    }
    while (exponent < 0)
    {
        value /= 16.0;
        ++exponent;
    }

    return static_cast<float>(value);
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
    case 1:
        return 4; // IBM float
    case 2:
        return 4; // int32
    case 3:
        return 2; // int16
    case 5:
        return 4; // IEEE float
    case 8:
        return 1; // int8
    default:
        throw std::runtime_error("unsupported SEG-Y sample format; supported formats: 1(IBM float), 2(int32), 3(int16), 5(IEEE float), 8(int8)");
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
            float sample_value = 0.0f;
            switch (binary_header_.data_sample_format)
            {
            case 1:
            {
                const std::uint32_t be_val = read_be_u32(input);
                sample_value = ibm_to_ieee_float(be_val);
                break;
            }
            case 2:
            {
                const std::uint32_t be_val = read_be_u32(input);
                sample_value = static_cast<float>(be_u32_to_i32(be_val));
                break;
            }
            case 3:
            {
                const std::uint16_t be_val = read_be_u16(input);
                sample_value = static_cast<float>(be_u16_to_i16(be_val));
                break;
            }
            case 5:
            {
                const std::uint32_t be_val = read_be_u32(input);
                sample_value = be_u32_to_ieee_float(be_val);
                break;
            }
            case 8:
            {
                signed char sample_i8 = 0;
                input.read(reinterpret_cast<char *>(&sample_i8), 1);
                if (!input)
                {
                    throw std::runtime_error("failed to read int8 from segy");
                }
                sample_value = static_cast<float>(sample_i8);
                break;
            }
            default:
                throw std::runtime_error("unsupported SEG-Y sample format while reading trace data");
            }
            output[trace * samples_per_trace + sample] = sample_value;
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
