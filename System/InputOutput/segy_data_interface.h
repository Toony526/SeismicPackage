#ifndef SEGY_DATA_INTERFACE_H
#define SEGY_DATA_INTERFACE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class SegyDataInterface
{
public:
    struct BinaryHeader
    {
        std::uint16_t sample_interval_us = 0;
        std::uint16_t num_samples_per_trace = 0;
        std::uint16_t data_sample_format = 0;
    };

    struct ConvertOptions
    {
        std::size_t traces_per_chunk = 2048;
        std::size_t decode_threads = 0; // 0 means auto(hardware_concurrency)
    };

    explicit SegyDataInterface(const std::string &file_path);

    BinaryHeader const & binary_header() const;
    std::size_t trace_count() const;
    std::vector<float> read_all_traces() const;

    void write_as_sep(const std::string &sep_header_path,
                      const std::string &sep_data_path) const;

    void write_as_sep(const std::string &sep_header_path,
                      const std::string &sep_data_path,
                      ConvertOptions const &options) const;

private:
    std::string file_path_;
    BinaryHeader binary_header_;
    std::size_t trace_count_ = 0;

    std::size_t bytes_per_sample() const;
    void parse_headers();
};

#endif
