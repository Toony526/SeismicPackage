#include "segy_data_interface.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <thread>
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

std::uint16_t read_be_u16_from_ptr(unsigned char const *ptr)
{
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(ptr[0]) << 8) |
                                      static_cast<std::uint16_t>(ptr[1]));
}

std::uint32_t read_be_u32_from_ptr(unsigned char const *ptr)
{
    return (static_cast<std::uint32_t>(ptr[0]) << 24) |
           (static_cast<std::uint32_t>(ptr[1]) << 16) |
           (static_cast<std::uint32_t>(ptr[2]) << 8) |
            static_cast<std::uint32_t>(ptr[3]);
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

float decode_sample(unsigned char const *ptr, std::uint16_t format)
{
    switch (format)
    {
    case 1:
        return ibm_to_ieee_float(read_be_u32_from_ptr(ptr));
    case 2:
        return static_cast<float>(be_u32_to_i32(read_be_u32_from_ptr(ptr)));
    case 3:
        return static_cast<float>(be_u16_to_i16(read_be_u16_from_ptr(ptr)));
    case 5:
        return be_u32_to_ieee_float(read_be_u32_from_ptr(ptr));
    case 8:
        return static_cast<float>(static_cast<signed char>(ptr[0]));
    default:
        throw std::runtime_error("unsupported SEG-Y sample format while decoding sample");
    }
}

void decode_trace_block_parallel(std::vector<unsigned char> const &raw_block,
                                 std::vector<float> &output,
                                 std::size_t traces_in_block,
                                 std::size_t samples_per_trace,
                                 std::size_t trace_size,
                                 std::size_t bytes_per_sample,
                                 std::uint16_t format,
                                 std::size_t decode_threads)
{
    if (traces_in_block == 0)
    {
        return;
    }

    if (decode_threads == 0)
    {
        decode_threads = std::thread::hardware_concurrency();
    }
    if (decode_threads == 0)
    {
        decode_threads = 1;
    }
    decode_threads = std::min(decode_threads, traces_in_block);

    auto worker = [&](std::size_t trace_begin, std::size_t trace_end)
    {
        for (std::size_t trace = trace_begin; trace < trace_end; ++trace)
        {
            const std::size_t trace_offset = trace * trace_size + 240;
            const std::size_t out_offset = trace * samples_per_trace;
            for (std::size_t sample = 0; sample < samples_per_trace; ++sample)
            {
                const std::size_t in_offset = trace_offset + sample * bytes_per_sample;
                output[out_offset + sample] = decode_sample(&raw_block[in_offset], format);
            }
        }
    };

    if (decode_threads == 1)
    {
        worker(0, traces_in_block);
        return;
    }

    std::vector<std::thread> pool;
    pool.reserve(decode_threads);
    const std::size_t chunk = (traces_in_block + decode_threads - 1) / decode_threads;
    for (std::size_t t = 0; t < decode_threads; ++t)
    {
        const std::size_t begin = t * chunk;
        if (begin >= traces_in_block)
        {
            break;
        }
        const std::size_t end = std::min(traces_in_block, begin + chunk);
        pool.push_back(std::thread(worker, begin, end));
    }

    for (std::size_t i = 0; i < pool.size(); ++i)
    {
        pool[i].join();
    }
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
    ConvertOptions options;
    write_as_sep(sep_header_path, sep_data_path, options);
}

void SegyDataInterface::write_as_sep(const std::string &sep_header_path,
                                     const std::string &sep_data_path,
                                     ConvertOptions const &options) const
{
    const std::size_t samples_per_trace = binary_header_.num_samples_per_trace;
    const std::size_t bytes_per_trace_sample = bytes_per_sample();
    const std::size_t trace_size = 240 + samples_per_trace * bytes_per_trace_sample;

    if (options.traces_per_chunk == 0)
    {
        throw std::runtime_error("invalid convert option: traces_per_chunk must be > 0");
    }

    std::ifstream input(file_path_, std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("failed to open SEG-Y file: " + file_path_);
    }

    std::vector<std::string> header_labels;
    header_labels.push_back("trace_index");

    std::vector<std::string> sort_order;
    sort_order.push_back("trace_index");

    SEPWriter writer(sep_header_path.c_str(),
                     0, static_cast<int>(binary_header_.sample_interval_us), static_cast<int>(samples_per_trace),
                     0, 1, static_cast<int>(trace_count_),
                     0, 1, 1,
                     header_labels,
                     sort_order,
                     sep_data_path.c_str());

    writer.OpenDataFile(sep_data_path.c_str());
    input.seekg(3600, std::ios::beg);

    std::size_t written_traces = 0;
    while (written_traces < trace_count_)
    {
        const std::size_t traces_in_chunk = std::min(options.traces_per_chunk, trace_count_ - written_traces);
        std::vector<unsigned char> raw_block(traces_in_chunk * trace_size, 0);
        input.read(reinterpret_cast<char *>(raw_block.data()), static_cast<std::streamsize>(raw_block.size()));
        if (input.gcount() != static_cast<std::streamsize>(raw_block.size()))
        {
            throw std::runtime_error("unexpected EOF while reading SEG-Y trace block");
        }

        std::vector<float> out_chunk(traces_in_chunk * samples_per_trace, 0.0f);
        decode_trace_block_parallel(raw_block,
                                    out_chunk,
                                    traces_in_chunk,
                                    samples_per_trace,
                                    trace_size,
                                    bytes_per_trace_sample,
                                    binary_header_.data_sample_format,
                                    options.decode_threads);

        writer.write_sepval(out_chunk.data(),
                            0,
                            static_cast<int>(written_traces),
                            0,
                            static_cast<int>(out_chunk.size()));

        written_traces += traces_in_chunk;
    }
}
