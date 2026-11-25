#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace echorestore::audio {

inline constexpr std::uint32_t wav_sample_rate = 16000;
inline constexpr std::uint16_t wav_channels = 1;

// Native integer values in [-32768, 32767], one value per mono sample.
// No normalization; zero means silence, not missing audio.
struct Pcm16Audio {
    std::vector<std::int16_t> samples;
    std::uint32_t sample_rate = wav_sample_rate;
    std::uint16_t channels = wav_channels;
};

// Only RIFF/WAVE format tag 1, 16 kHz, mono, signed PCM16 is supported.
// Throws std::runtime_error for invalid/unsupported files or I/O failures.
[[nodiscard]] Pcm16Audio read_wav(const std::filesystem::path& path);

// Writes canonical little-endian PCM16, without preserving input metadata.
// Creates parent directories and overwrites the destination (not atomic).
// Validates format and RIFF size limits before opening the destination.
void write_wav(const std::filesystem::path& path, const Pcm16Audio& audio);

}  // namespace echorestore::audio
