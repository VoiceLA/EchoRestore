#include "echorestore/audio/wav.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace echorestore::audio {
namespace {

using Bytes = std::array<unsigned char, 12>;

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error("WAV: " + std::string(message));
}

void read_exact(std::istream &input, unsigned char *bytes, std::size_t count) {
  if (!input.read(reinterpret_cast<char *>(bytes),
                  static_cast<std::streamsize>(count)))
    fail("truncated file or read failure");
}

std::uint32_t read_le(const unsigned char *bytes, unsigned count) {
  std::uint32_t value = 0;

  for (unsigned i = 0; i < count; ++i) {
    value |= static_cast<std::uint32_t>(bytes[i]) << (8 * i);
  }

  return value;
}

bool has_id(const unsigned char *bytes, std::string_view id) {
  return std::equal(id.begin(), id.end(), bytes);
}

void seek(std::istream &input, std::uint64_t position) {
  if (position >
      static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
    fail("file offset exceeds stream limits");

  input.seekg(static_cast<std::streamoff>(position), std::ios::beg);

  if (!input)
    fail("cannot seek in input file");
}

void write_le(std::ostream &output, std::uint32_t value, unsigned count) {
  std::array<unsigned char, 4> bytes{};

  for (unsigned i = 0; i < count; ++i) {
    bytes[i] = static_cast<unsigned char>((value >> (8 * i)) & 0xffU);
  }

  output.write(reinterpret_cast<const char *>(bytes.data()), count);
}

void validate_format(const unsigned char *format) {
  if (read_le(format, 2) != 1 || read_le(format + 2, 2) != wav_channels ||
      read_le(format + 4, 4) != wav_sample_rate ||
      read_le(format + 14, 2) != 16)
    fail("unsupported format; expected format tag 1, 16000 Hz, mono, signed "
         "PCM16");

  if (read_le(format + 8, 4) != wav_sample_rate * 2 ||
      read_le(format + 12, 2) != 2)
    fail("invalid PCM16 byte rate or block alignment");
}

} // namespace

Pcm16Audio read_wav(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);

  if (!input)
    fail("cannot open input: " + path.string());

  const auto end = input.tellg();

  if (end < std::streampos{12})
    fail("file is shorter than the RIFF header");

  const auto file_size =
      static_cast<std::uint64_t>(static_cast<std::streamoff>(end));
  seek(input, 0);
  Bytes header{};
  read_exact(input, header.data(), header.size());

  if (!has_id(header.data(), "RIFF") || !has_id(header.data() + 8, "WAVE"))
    fail("expected little-endian RIFF/WAVE container");

  const std::uint64_t riff_end =
      std::uint64_t{read_le(header.data() + 4, 4)} + 8;

  if (riff_end != file_size)
    fail("RIFF size does not match file length");

  bool found_format = false;
  bool found_data = false;
  std::uint64_t data_offset = 0;
  std::uint32_t data_bytes = 0;

  for (std::uint64_t offset = 12; offset < riff_end;) {
    if (riff_end - offset < 8)
      fail("incomplete chunk header");

    seek(input, offset);
    read_exact(input, header.data(), 8);
    const auto chunk_size = read_le(header.data() + 4, 4);
    const std::uint64_t payload_offset = offset + 8;
    const std::uint64_t padded_size =
        std::uint64_t{chunk_size} + (chunk_size % 2);

    if (padded_size > riff_end - payload_offset)
      fail("chunk extends beyond RIFF boundary or lacks padding");

    if (has_id(header.data(), "fmt ")) {
      if (found_format)
        fail("duplicate fmt chunk");

      // PCM commonly uses 16 bytes or WAVEFORMATEX with cbSize = 0.

      if (chunk_size != 16 && chunk_size != 18)
        fail("unsupported fmt chunk size; expected 16 or 18 bytes");

      std::array<unsigned char, 18> format{};
      read_exact(input, format.data(), chunk_size);
      validate_format(format.data());

      if (chunk_size == 18 && read_le(format.data() + 16, 2) != 0)
        fail("unsupported PCM format extension");

      found_format = true;
    } else if (has_id(header.data(), "data")) {
      if (found_data)
        fail("duplicate data chunk");

      if (chunk_size % 2 != 0)
        fail("PCM16 data contains an incomplete sample");

      data_offset = payload_offset;
      data_bytes = chunk_size;
      found_data = true;
    }

    offset = payload_offset + padded_size;
  }

  if (!found_format || !found_data)
    fail("required fmt or data chunk is missing");

  Pcm16Audio audio;
  const auto sample_count = static_cast<std::size_t>(data_bytes / 2);

  if (sample_count > audio.samples.max_size())
    fail("sample count exceeds buffer limits");

  audio.samples.resize(sample_count);
  seek(input, data_offset);

  std::array<unsigned char, 8192> buffer{};

  for (std::size_t offset = 0; offset < audio.samples.size();) {
    const auto count =
        std::min(buffer.size() / 2, audio.samples.size() - offset);

    read_exact(input, buffer.data(), count * 2);

    for (std::size_t i = 0; i < count; ++i) {
      const auto bits = read_le(buffer.data() + i * 2, 2);
      const auto value = bits < 0x8000U
                             ? static_cast<std::int32_t>(bits)
                             : static_cast<std::int32_t>(bits) - 65536;
      audio.samples[offset + i] = static_cast<std::int16_t>(value);
    }

    offset += count;
  }

  return audio;
}

void write_wav(const std::filesystem::path &path, const Pcm16Audio &audio) {
  if (audio.sample_rate != wav_sample_rate || audio.channels != wav_channels)
    fail("unsupported output format; expected 16000 Hz, mono, signed PCM16");

  constexpr auto max_samples =
      (std::numeric_limits<std::uint32_t>::max() - 36U) / 2U;

  if (audio.samples.size() > max_samples)
    fail("audio exceeds RIFF 32-bit size limit");

  const auto data_bytes = static_cast<std::uint32_t>(audio.samples.size()) * 2U;

  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path());

  std::ofstream output(path, std::ios::binary | std::ios::trunc);

  if (!output)
    fail("cannot open output: " + path.string());

  output.write("RIFF", 4);
  write_le(output, 36U + data_bytes, 4);
  output.write("WAVEfmt ", 8);
  write_le(output, 16, 4);
  write_le(output, 1, 2);
  write_le(output, wav_channels, 2);
  write_le(output, wav_sample_rate, 4);
  write_le(output, wav_sample_rate * 2, 4);
  write_le(output, 2, 2);
  write_le(output, 16, 2);

  output.write("data", 4);
  write_le(output, data_bytes, 4);

  std::array<unsigned char, 8192> buffer{};

  for (std::size_t offset = 0; offset < audio.samples.size();) {
    const auto count =
        std::min(buffer.size() / 2, audio.samples.size() - offset);

    for (std::size_t i = 0; i < count; ++i) {
      const auto bits = static_cast<std::uint16_t>(audio.samples[offset + i]);
      buffer[i * 2] = static_cast<unsigned char>(bits & 0xffU);
      buffer[i * 2 + 1] = static_cast<unsigned char>(bits >> 8);
    }

    output.write(reinterpret_cast<const char *>(buffer.data()),
                 static_cast<std::streamsize>(count * 2));

    if (!output)
      fail("failed to write PCM data: " + path.string());

    offset += count;
  }

  output.close();

  if (!output)
    fail("failed to finish output: " + path.string());
}

} // namespace echorestore::audio
