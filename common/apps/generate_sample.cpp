#include "echorestore/audio/wav.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

void generate(const std::filesystem::path &path) {
  constexpr std::uint32_t total_samples = 16000;

  echorestore::audio::Pcm16Audio audio;
  audio.samples.reserve(total_samples);

  for (std::uint32_t i = 0; i < total_samples; ++i) {
    // Integer-only 400 Hz square wave, amplitude 8000, reproducible bytes.
    const std::int16_t sample = (i % 40 < 20) ? 8000 : -8000;
    audio.samples.push_back(sample);
  }

  echorestore::audio::write_wav(path, audio);
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc == 2 && std::string_view{argv[1]} == "--help") {
    std::cout << "Usage: echorestore_generate_sample OUTPUT.wav\n"
              << "Write a deterministic 1-second 16 kHz mono PCM16 fixture.\n"
              << "Creates parent directories; overwrites OUTPUT.wav.\n";

    return 0;
  }

  if (argc != 2 || std::string_view{argv[1]}.empty() ||
      std::string_view{argv[1]}.starts_with("--")) {
    std::cerr << "Usage: echorestore_generate_sample OUTPUT.wav\n";

    return 1;
  }

  try {
    generate(argv[1]);

    std::cout << "Created " << argv[1] << " (16000 samples).\n";

    return 0;
  } catch (const std::exception &error) {
    std::cerr << "Sample generation failed: " << error.what() << '\n';

    return 1;
  }
}
